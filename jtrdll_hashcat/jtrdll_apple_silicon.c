/*
 * jtrdll_apple_silicon.c
 * Implements the jtrdll API as a wrapper around hashcat for Apple Silicon.
 * Builds to: jtrdll_apple-silicon.dylib
 *
 * API expected by lc7jtr plugin (from jtrdll.h):
 *   jtrdll_main, jtrdll_abort, jtrdll_get_status,
 *   jtrdll_get_charset_info, jtrdll_cleanup,
 *   jtrdll_preflight, jtrdll_set_extra_opencl_kernel_args
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <spawn.h>

#include "../external/jtrdll/jtrdll/jtrdll.h"

/* ============================================================
 * Configuration
 * ============================================================ */
#define HASHCAT_BIN_DEFAULT "/usr/local/bin/hashcat"
#define HASHCAT_RULES "/usr/local/share/doc/hashcat/rules"

/* ============================================================
 * Hash format mapping: JtR format name → hashcat -m mode
 * ============================================================ */
typedef struct { const char *jtr; int hc_mode; } fmt_map_t;

static const fmt_map_t s_fmt[] = {
    {"NT",                   1000},  /* NTLM                  */
    {"LM",                   3000},  /* LM                    */
    {"netlm",                  23},  /* NetLM                 */
    {"netntlm",              5500},  /* NetNTLMv1             */
    {"netntlmv2",            5600},  /* NetNTLMv2             */
    {"descrypt",             1500},  /* DES-crypt             */
    {"md5crypt",              500},  /* md5crypt ($1$)        */
    {"bcrypt",               3200},  /* bcrypt ($2a$)         */
    {"sha256crypt",          7400},  /* sha256crypt ($5$)     */
    {"sha512crypt",          1800},  /* sha512crypt ($6$)     */
    {"aix-smd5",             6300},  /* AIX {smd5}            */
    {"aix-ssha1",            6700},  /* AIX {ssha1}           */
    {"aix-ssha256",          6400},  /* AIX {ssha256}         */
    {"aix-ssha512",          6500},  /* AIX {ssha512}         */
    {"xsha",                  122},  /* macOS v10.4-10.6 SHA1 */
    {"xsha512",              1722},  /* macOS v10.8+ SHA512   */
    {"PBKDF2-HMAC-SHA512",   7100},  /* macOS PBKDF2          */
    {NULL, -1}
};

static int jtr_format_to_hc_mode(const char *fmt)
{
    for (int i = 0; s_fmt[i].jtr; i++)
        if (strcasecmp(s_fmt[i].jtr, fmt) == 0)
            return s_fmt[i].hc_mode;
    return -1;
}

/* ============================================================
 * Global state (protected by g_mutex)
 * ============================================================ */
static pthread_mutex_t g_mutex    = PTHREAD_MUTEX_INITIALIZER;
static volatile pid_t  g_pid      = -1;
static volatile int    g_stage    = 0;   /* 0=idle 1=init 2=running 3=done */
static volatile double g_pct      = 0.0;
static volatile double g_elapsed  = 0.0;
static volatile double g_eta      = 0.0;
static volatile time_t g_start_time = 0; /* wall-clock start of current pass */
static volatile uint64_t g_guesses = 0;
static volatile uint64_t g_cands   = 0;
static volatile double g_speed    = 0.0;
static char g_word1[256] = "";
static char g_word2[256] = "";

/* ============================================================
 * Hash map: hc_hash → jtr_potfile_hash
 * Used to convert hashcat outfile entries → JtR potfile format
 * ============================================================ */
typedef struct { char hc[512]; char jtr[512]; } hmap_entry_t;
static hmap_entry_t *g_hmap     = NULL;
static int           g_hmap_cnt = 0;
static int           g_hmap_cap = 0;

static void hmap_reset(void) { g_hmap_cnt = 0; }

static void hmap_add(const char *hc, const char *jtr)
{
    if (g_hmap_cnt >= g_hmap_cap) {
        int nc = g_hmap_cap ? g_hmap_cap * 2 : 1024;
        hmap_entry_t *p = realloc(g_hmap, nc * sizeof(*p));
        if (!p) return;
        g_hmap = p;
        g_hmap_cap = nc;
    }
    snprintf(g_hmap[g_hmap_cnt].hc,  sizeof(g_hmap[0].hc),  "%s", hc);
    snprintf(g_hmap[g_hmap_cnt].jtr, sizeof(g_hmap[0].jtr), "%s", jtr);
    g_hmap_cnt++;
}

/* Lookup: given a hc_hash string, return jtr_hash or NULL */
static const char *hmap_lookup_by_hc(const char *hc)
{
    /* First try exact lowercase match (fast path for simple hashes) */
    for (int i = 0; i < g_hmap_cnt; i++)
        if (strcasecmp(g_hmap[i].hc, hc) == 0)
            return g_hmap[i].jtr;
    return NULL;
}

/* Lookup: given an output line "hash:plain", find jtr_hash and set *plain_start.
 * Handles hashes that contain ':' (e.g. NetNTLMv2). */
static const char *hmap_find_in_line(const char *line, const char **plain_start)
{
    /* Iterate all map entries, check if line starts with hc hash */
    for (int i = 0; i < g_hmap_cnt; i++) {
        size_t hlen = strlen(g_hmap[i].hc);
        if (strncasecmp(line, g_hmap[i].hc, hlen) == 0 && line[hlen] == ':') {
            *plain_start = line + hlen + 1;
            return g_hmap[i].jtr;
        }
    }
    /* Fallback: split on first ':' */
    const char *colon = strchr(line, ':');
    if (!colon) return NULL;
    *plain_start = colon + 1;
    /* Create a temp key from start to colon */
    static char tmp_key[512];
    int klen = (int)(colon - line);
    if (klen >= (int)sizeof(tmp_key)) klen = (int)sizeof(tmp_key) - 1;
    strncpy(tmp_key, line, klen);
    tmp_key[klen] = '\0';
    const char *r = hmap_lookup_by_hc(tmp_key);
    if (r) return r;
    return tmp_key; /* fallback: use hc hash as jtr hash */
}

/* ============================================================
 * Utility
 * ============================================================ */
static void rtrim(char *s)
{
    int l = (int)strlen(s);
    while (l > 0 && (s[l-1] == '\n' || s[l-1] == '\r' || s[l-1] == ' '))
        s[--l] = '\0';
}

static void str_lower(char *s)
{
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

/* Forward declarations for argv builder used by helpers */
static void hcarg_add(const char *s);
static void hcarg_addf(const char *fmt, ...);

static const char *resolve_hashcat_bin(void)
{
    static char resolved[PATH_MAX] = {0};
    if (resolved[0]) return resolved;

    const char *candidates[] = {
        "/opt/homebrew/bin/hashcat",  /* native Apple Silicon Homebrew */
        "/usr/local/bin/hashcat",     /* Intel/Homebrew fallback */
        HASHCAT_BIN_DEFAULT,
        NULL
    };
    for (int i = 0; candidates[i]; i++) {
        if (access(candidates[i], X_OK) == 0) {
            snprintf(resolved, sizeof(resolved), "%s", candidates[i]);
            return resolved;
        }
    }

    /* Last resort: rely on PATH lookup by posix_spawnp. */
    snprintf(resolved, sizeof(resolved), "%s", "hashcat");
    return resolved;
}

static void add_hashcat_rule_if_any(const char *jtr_rules)
{
    if (!jtr_rules || !jtr_rules[0]) return;

    /* LC7/JtR passes either a rule set name (best64) or a concrete .rule file path (Finalyse/buka_400k). */
    if (strchr(jtr_rules, '/') != NULL || strchr(jtr_rules, '\\') != NULL) {
        if (access(jtr_rules, R_OK) == 0) {
            hcarg_addf("-r%s", jtr_rules);
        }
        return;
    }

    /* Known named rulesets shipped with hashcat */
    if (strcasecmp(jtr_rules, "best64") == 0 || strcasecmp(jtr_rules, "Best64") == 0) {
        char rules_path[PATH_MAX];
        snprintf(rules_path, PATH_MAX, "%s/best64.rule", HASHCAT_RULES);
        if (access(rules_path, R_OK) == 0) {
            hcarg_addf("-r%s", rules_path);
        }
        return;
    }
}

/* ============================================================
 * Hash conversion: JtR hash field → hashcat input hash
 * Populates hc_out (hashcat input) and jtr_out (potfile key).
 * Returns 1 on success, 0 to skip this hash.
 * ============================================================ */
static int jtr_to_hc_hash(const char *jtr_field, int hc_mode,
                           char *hc_out, size_t hc_sz,
                           char *jtr_out, size_t jtr_sz)
{
    snprintf(jtr_out, jtr_sz, "%s", jtr_field);

    switch (hc_mode) {
    case 1000: /* NTLM: strip $NT$ prefix */
        if (strncmp(jtr_field, "$NT$", 4) == 0)
            snprintf(hc_out, hc_sz, "%s", jtr_field + 4);
        else
            snprintf(hc_out, hc_sz, "%s", jtr_field);
        return 1;

    case 3000: /* LM: strip $LM$ prefix */
        if (strncmp(jtr_field, "$LM$", 4) == 0)
            snprintf(hc_out, hc_sz, "%s", jtr_field + 4);
        else
            snprintf(hc_out, hc_sz, "%s", jtr_field);
        return 1;

    case 500:  case 3200: case 7400: case 1800: case 1500:
    case 6300: case 6700: case 6400: case 6500:
    case 122:  case 1722: case 7100:
        /* Same format for JtR and hashcat */
        snprintf(hc_out, hc_sz, "%s", jtr_field);
        return 1;

    case 23:   /* NetLM: JtR $NETLM$chal16$resp48 */
    {
        if (strncmp(jtr_field, "$NETLM$", 7) != 0) return 0;
        const char *chal = jtr_field + 7;
        const char *d    = strchr(chal, '$');
        if (!d) return 0;
        char chal_hex[17] = {0};
        int clen = (int)(d - chal);
        if (clen > 16) clen = 16;
        strncpy(chal_hex, chal, clen);
        const char *resp = d + 1;
        /* hashcat NetLM format: user::DOMAIN:challenge:LMResponse:NTResponse */
        /* We simplify: use dummy user/domain; works for cracking */
        snprintf(hc_out, hc_sz, "user::WORKGROUP:%s:%s:%s",
                 chal_hex, resp, resp);
        return 1;
    }

    case 5500: /* NetNTLMv1: JtR $NETNTLM$chal16$resp48 */
    {
        if (strncmp(jtr_field, "$NETNTLM$", 9) != 0) return 0;
        const char *chal = jtr_field + 9;
        const char *d    = strchr(chal, '$');
        if (!d) return 0;
        char chal_hex[17] = {0};
        int clen = (int)(d - chal);
        if (clen > 16) clen = 16;
        strncpy(chal_hex, chal, clen);
        const char *lmresp = d + 1;
        const char *ntresp = d + 1; /* same for v1 challenge */
        /* hashcat format: user::domain:LMResponse:NTResponse:challenge */
        snprintf(hc_out, hc_sz, "user::WORKGROUP:%s:%s:%s",
                 lmresp, ntresp, chal_hex);
        return 1;
    }

    case 5600: /* NetNTLMv2: JtR $NETNTLMv2$USERDOMAIN$schal$resp$cchal */
    {
        if (strncmp(jtr_field, "$NETNTLMv2$", 11) != 0) return 0;
        const char *p  = jtr_field + 11;
        const char *d1 = strchr(p, '$');   if (!d1) return 0;
        /* user+domain portion */
        char userdomain[128] = {0};
        int udlen = (int)(d1 - p);
        if (udlen > 127) udlen = 127;
        strncpy(userdomain, p, udlen);

        const char *schal = d1 + 1;
        const char *d2 = strchr(schal, '$'); if (!d2) return 0;
        char schal_hex[17] = {0};
        strncpy(schal_hex, schal, 16);

        const char *resp = d2 + 1;
        const char *d3 = strchr(resp, '$'); if (!d3) return 0;
        char ntproof[33] = {0};
        strncpy(ntproof, resp, 32);

        /* rest of response (after NTProofStr) + cchal */
        char blob[512] = {0};
        if (strlen(resp) > 32)
            snprintf(blob, sizeof(blob), "%s%s", resp + 32, d3 + 1);
        else
            snprintf(blob, sizeof(blob), "%s", d3 + 1);

        /* hashcat NetNTLMv2: user::domain:ServerChallenge:NTProofStr:blob */
        snprintf(hc_out, hc_sz, "%s::WORKGROUP:%s:%s:%s",
                 userdomain, schal_hex, ntproof, blob);
        return 1;
    }

    default:
        snprintf(hc_out, hc_sz, "%s", jtr_field);
        return 1;
    }
}

/* Convert JtR hash file → hashcat hash file; populate hash map.
 * Returns number of hashes written, -1 on error. */
static int convert_hashfile(const char *jtr_path, const char *hc_path, int hc_mode)
{
    FILE *fin = fopen(jtr_path, "r");
    if (!fin) return -1;
    FILE *fout = fopen(hc_path, "w");
    if (!fout) { fclose(fin); return -1; }

    hmap_reset();
    int count = 0;
    char line[2048];

    while (fgets(line, sizeof(line), fin)) {
        rtrim(line);
        if (!line[0]) continue;

        /* username:hash[:...] */
        char *c1 = strchr(line, ':');
        if (!c1) continue;
        char *hash_start = c1 + 1;
        char *c2 = strchr(hash_start, ':');

        char jtr_field[512] = {0};
        if (c2) {
            int len = (int)(c2 - hash_start);
            if (len > 511) len = 511;
            strncpy(jtr_field, hash_start, len);
        } else {
            strncpy(jtr_field, hash_start, 511);
        }
        if (!jtr_field[0]) continue;

        char hc_hash[512]  = {0};
        char jtr_key[512]  = {0};
        if (!jtr_to_hc_hash(jtr_field, hc_mode,
                             hc_hash,  sizeof(hc_hash),
                             jtr_key,  sizeof(jtr_key)))
            continue;

        /* Store lowercase hc_hash in map */
        char hc_lower[512];
        strncpy(hc_lower, hc_hash, 511);
        str_lower(hc_lower);

        hmap_add(hc_lower, jtr_key);
        fprintf(fout, "%s\n", hc_hash);
        count++;
    }
    fclose(fin);
    fclose(fout);
    return count;
}

/* ============================================================
 * Single mode helpers
 * ============================================================ */

/* Emit one base word with all variations + number/symbol suffixes */
static void emit_word(FILE *fout, const char *word)
{
    if (!word || !word[0] || strlen(word) < 2) return;

    static const char *sfx[] = {
        "","1","2","3","12","21","123","321","1234","12345","123456",
        "01","02","99","00","007","111","222","333","!","@","#",
        "1!","2@","123!","!@#","2023","2024","2025","_1","_123",NULL
    };

    char low[128], cap[128], up[128];
    int  l = (int)strlen(word);
    if (l > 127) l = 127;

    strncpy(low, word, l); low[l] = '\0';
    str_lower(low);

    strncpy(cap, low, 127);
    if (cap[0]) cap[0] = (char)toupper((unsigned char)cap[0]);

    strncpy(up, word, l); up[l] = '\0';
    for (int i = 0; up[i]; i++) up[i] = (char)toupper((unsigned char)up[i]);

    for (int s = 0; sfx[s]; s++) {
        /* cap+suffix  (most common: Password123) */
        fprintf(fout, "%s%s\n", cap, sfx[s]);
        /* low+suffix  (password123) */
        if (strcmp(low, cap) != 0)
            fprintf(fout, "%s%s\n", low, sfx[s]);
        /* up+suffix   (PASSWORD123) */
        if (strcmp(up, cap) != 0 && strcmp(up, low) != 0)
            fprintf(fout, "%s%s\n", up, sfx[s]);
    }
}

/* Split a string by delimiters, emit each token >= min_len */
static void split_and_emit(FILE *fout, const char *str, const char *delims, int min_len)
{
    char buf[256];
    strncpy(buf, str, 255);
    char *tok = strtok(buf, delims);
    while (tok) {
        if ((int)strlen(tok) >= min_len)
            emit_word(fout, tok);
        tok = strtok(NULL, delims);
    }
}

/* ============================================================
 * Single mode: generate wordlist from JtR hash file.
 * JtR hash file format: username:hash:::Full Name::
 * Fields:  [0]=username [1]=hash [2]='' [3]='' [4]=fullname [5]='' [6]=''
 * ============================================================ */
static int create_single_wordlist(const char *jtr_hashfile, const char *out_path)
{
    FILE *fin  = fopen(jtr_hashfile, "r");
    if (!fin) return -1;
    FILE *fout = fopen(out_path, "w");
    if (!fout) { fclose(fin); return -1; }

    char line[2048];
    int  count = 0;

    while (fgets(line, sizeof(line), fin)) {
        rtrim(line);
        if (!line[0]) continue;

        /* --- field 0: username --- */
        char *c1 = strchr(line, ':');
        if (!c1) continue;
        int ulen = (int)(c1 - line);
        if (ulen <= 0 || ulen > 127) continue;

        char username[128] = {0};
        strncpy(username, line, ulen);

        /* whole username */
        emit_word(fout, username);

        /* split on . - _ (e.g. "j.akmalov" → "j", "akmalov") */
        split_and_emit(fout, username, "._-", 3);

        /* --- field 4: Full Name (after 4 colons) --- */
        /* format: username:hash:::fullname:: */
        char *p = c1; /* after username */
        int colons = 0;
        char *fullname_start = NULL;
        while (*p) {
            if (*p == ':') {
                colons++;
                if (colons == 4) { fullname_start = p + 1; break; }
            }
            p++;
        }
        if (fullname_start && fullname_start[0] && fullname_start[0] != ':') {
            /* extract fullname up to next ':' */
            char fullname[256] = {0};
            char *c_end = strchr(fullname_start, ':');
            int fnlen = c_end ? (int)(c_end - fullname_start) : (int)strlen(fullname_start);
            if (fnlen > 0 && fnlen < 255) {
                strncpy(fullname, fullname_start, fnlen);
                /* whole full name */
                emit_word(fout, fullname);
                /* split on spaces, dots, dashes */
                split_and_emit(fout, fullname, " ._-", 3);
            }
        }
        count++;
    }
    fclose(fin);
    fclose(fout);
    return count;
}

/* ============================================================
 * Potfile update: read new lines from hashcat potfile,
 * convert and append to JtR potfile.
 * ============================================================ */
static off_t  s_hc_pot_offset = 0;
static char   s_hc_potfile_read[PATH_MAX] = ""; /* hashcat potfile (hash:plain) */
static char   s_jtr_potfile[PATH_MAX] = "";     /* JtR potfile ($NT$hash:plain) */

static void flush_cracked(void)
{
    if (!s_hc_potfile_read[0] || !s_jtr_potfile[0]) return;

    FILE *fin = fopen(s_hc_potfile_read, "r");
    if (!fin) return;
    if (fseeko(fin, s_hc_pot_offset, SEEK_SET) != 0) { fclose(fin); return; }

    FILE *fpot = fopen(s_jtr_potfile, "a");
    if (!fpot) { fclose(fin); return; }

    char line[1024];
    while (fgets(line, sizeof(line), fin)) {
        rtrim(line);
        if (!line[0]) continue;

        const char *plain    = NULL;
        const char *jtr_hash = hmap_find_in_line(line, &plain);

        if (jtr_hash && plain)
            fprintf(fpot, "%s:%s\n", jtr_hash, plain);
    }
    s_hc_pot_offset = ftello(fin);
    fclose(fin);
    fclose(fpot);
}

/* ============================================================
 * Status output parsing (hashcat stdout lines)
 * ============================================================ */
static void parse_status_line(const char *line)
{
    if (!line || !line[0]) return;

    /* Progress.........: 1234/12345678 (0.00%) */
    if (strstr(line, "Progress")) {
        const char *paren = strchr(line, '(');
        if (paren) {
            double pct = 0.0;
            if (sscanf(paren + 1, "%lf", &pct) == 1) {
                pthread_mutex_lock(&g_mutex);
                g_pct = pct;
                pthread_mutex_unlock(&g_mutex);
            }
        }
        /* also grab counts */
        const char *col = strchr(line, ':');
        if (col) {
            uint64_t done = 0, total = 0;
            if (sscanf(col + 1, " %" SCNu64 "/%" SCNu64, &done, &total) == 2) {
                pthread_mutex_lock(&g_mutex);
                g_guesses = done;
                g_cands   = total;
                pthread_mutex_unlock(&g_mutex);
            }
        }
    }
    /* Speed.#1.........: 1234.5 MH/s */
    else if (strstr(line, "Speed.#") || strstr(line, "Speed:#")) {
        const char *col = strchr(line, ':');
        if (col) {
            double sp = 0.0;
            char unit[16] = "";
            if (sscanf(col + 1, " %lf %15s", &sp, unit) >= 1) {
                if (strncmp(unit, "kH", 2) == 0) sp *= 1e3;
                else if (strncmp(unit, "MH", 2) == 0) sp *= 1e6;
                else if (strncmp(unit, "GH", 2) == 0) sp *= 1e9;
                else if (strncmp(unit, "TH", 2) == 0) sp *= 1e12;
                pthread_mutex_lock(&g_mutex);
                g_speed = sp;
                pthread_mutex_unlock(&g_mutex);
            }
        }
    }
    /* Time.Running.....: X secs / X mins */
    else if (strstr(line, "Time.Running")) {
        const char *col = strchr(line, ':');
        if (col) {
            double t = 0.0;
            char unit[16] = "";
            if (sscanf(col + 1, " %lf %15s", &t, unit) == 2) {
                if (strncmp(unit, "min", 3) == 0) t *= 60.0;
                else if (strncmp(unit, "hour", 4) == 0) t *= 3600.0;
                pthread_mutex_lock(&g_mutex);
                g_elapsed = t;
                pthread_mutex_unlock(&g_mutex);
            }
        }
    }
    /* Candidates.#1....: abc -> xyz */
    else if (strstr(line, "Candidates")) {
        const char *arrow = strstr(line, " -> ");
        const char *col   = strchr(line, ':');
        if (arrow && col && col < arrow) {
            char w1[256] = {0}, w2[256] = {0};
            int l1 = (int)(arrow - col - 1);
            if (l1 > 0 && l1 < 255) {
                strncpy(w1, col + 1, l1);
                /* trim leading space */
                char *p = w1;
                while (*p == ' ') p++;
                if (p != w1) memmove(w1, p, strlen(p) + 1);
            }
            strncpy(w2, arrow + 4, 255);
            pthread_mutex_lock(&g_mutex);
            strncpy(g_word1, w1, 255);
            strncpy(g_word2, w2, 255);
            pthread_mutex_unlock(&g_mutex);
        }
    }
}

/* ============================================================
 * Arg helpers
 * ============================================================ */
static const char *find_arg(int argc, char **argv, const char *prefix)
{
    int plen = (int)strlen(prefix);
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], prefix, plen) == 0) {
            const char *v = argv[i] + plen;
            return (*v == '=') ? v + 1 : v;
        }
    }
    return NULL;
}

static int has_arg(int argc, char **argv, const char *name)
{
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], name) == 0) return 1;
    return 0;
}

/* ============================================================
 * Hashcat argv builder
 * ============================================================ */
#define MAX_HC_ARGS 128
static char  s_hc_strs[MAX_HC_ARGS][PATH_MAX];
static char *s_hc_argv[MAX_HC_ARGS + 1];
static int   s_hc_argc = 0;

static void hcarg_reset(void)   { s_hc_argc = 0; }

static void hcarg_add(const char *s)
{
    if (s_hc_argc >= MAX_HC_ARGS) return;
    strncpy(s_hc_strs[s_hc_argc], s, PATH_MAX - 1);
    s_hc_strs[s_hc_argc][PATH_MAX - 1] = '\0';
    s_hc_argv[s_hc_argc] = s_hc_strs[s_hc_argc];
    s_hc_argc++;
    s_hc_argv[s_hc_argc] = NULL;
}

static void hcarg_addf(const char *fmt, ...)
{
    char buf[PATH_MAX];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, PATH_MAX, fmt, ap);
    va_end(ap);
    hcarg_add(buf);
}

/* ============================================================
 * Temp file management
 * ============================================================ */
#define MAX_TMP 16
static char s_tmpfiles[MAX_TMP][PATH_MAX];
static int  s_tmpcount = 0;

static const char *make_tmpfile(const char *suffix)
{
    if (s_tmpcount >= MAX_TMP) return NULL;
    snprintf(s_tmpfiles[s_tmpcount], PATH_MAX,
             "/tmp/jtrhc_%d_%d%s", (int)getpid(), s_tmpcount, suffix);
    return s_tmpfiles[s_tmpcount++];
}

static void cleanup_tmpfiles(void)
{
    for (int i = 0; i < s_tmpcount; i++) unlink(s_tmpfiles[i]);
    s_tmpcount = 0;
}

/* ============================================================
 * Reader thread: reads fd line-by-line, parses status, calls hooks
 * ============================================================ */
struct reader_ctx {
    int                  fd;
    struct JTRDLL_HOOKS *hooks;
    int                  is_stderr;
};

static void *reader_thread(void *arg)
{
    struct reader_ctx *ctx = (struct reader_ctx *)arg;
    char buf[4096];
    char linebuf[4096];
    int  linelen = 0;

    while (1) {
        ssize_t n = read(ctx->fd, buf, sizeof(buf) - 1);
        if (n <= 0) break;
        buf[n] = '\0';

        for (ssize_t i = 0; i < n; i++) {
            char c = buf[i];
            if (c == '\r') continue;
            if (c == '\n' || linelen >= (int)sizeof(linebuf) - 2) {
                linebuf[linelen] = '\0';
                parse_status_line(linebuf);
                flush_cracked();
                if (linebuf[0]) {
                    if (ctx->is_stderr) {
                        if (ctx->hooks && ctx->hooks->stderr_hook)
                            ctx->hooks->stderr_hook(ctx->hooks->ctx, linebuf);
                    } else {
                        if (ctx->hooks && ctx->hooks->stdout_hook)
                            ctx->hooks->stdout_hook(ctx->hooks->ctx, linebuf);
                    }
                }
                linelen = 0;
            } else {
                linebuf[linelen++] = c;
            }
        }
    }
    /* flush remaining */
    if (linelen > 0) {
        linebuf[linelen] = '\0';
        parse_status_line(linebuf);
        if (ctx->hooks && ctx->hooks->stdout_hook)
            ctx->hooks->stdout_hook(ctx->hooks->ctx, linebuf);
    }
    close(ctx->fd);
    free(ctx);
    return NULL;
}

/* ============================================================
 * Execute hashcat (fork/exec)
 * Returns hashcat exit code (0=cracked/exhausted, others=error)
 * ============================================================ */
extern char **environ;

static int run_hashcat(struct JTRDLL_HOOKS *hooks)
{
    int pout[2], perr[2];
    if (pipe(pout) != 0 || pipe(perr) != 0) return -1;

    /* Use posix_spawn — safe in multi-threaded processes on macOS */
    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_adddup2(&fa, pout[1], STDOUT_FILENO);
    posix_spawn_file_actions_adddup2(&fa, perr[1], STDERR_FILENO);
    posix_spawn_file_actions_addclose(&fa, pout[0]);
    posix_spawn_file_actions_addclose(&fa, pout[1]);
    posix_spawn_file_actions_addclose(&fa, perr[0]);
    posix_spawn_file_actions_addclose(&fa, perr[1]);

    pid_t pid = -1;
    const char *hashcat_bin = resolve_hashcat_bin();
    int rc = posix_spawnp(&pid, hashcat_bin, &fa, NULL, s_hc_argv, environ);
    posix_spawn_file_actions_destroy(&fa);

    close(pout[1]);
    close(perr[1]);

    if (rc != 0) {
        close(pout[0]);
        close(perr[0]);
        return -1;
    }

    /* parent */

    pthread_mutex_lock(&g_mutex);
    g_pid        = pid;
    g_stage      = 2;
    g_start_time = time(NULL);
    pthread_mutex_unlock(&g_mutex);

    struct reader_ctx *rout = malloc(sizeof(*rout));
    struct reader_ctx *rerr = malloc(sizeof(*rerr));
    rout->fd = pout[0]; rout->hooks = hooks; rout->is_stderr = 0;
    rerr->fd = perr[0]; rerr->hooks = hooks; rerr->is_stderr = 1;

    pthread_t tout, terr;
    pthread_create(&tout, NULL, reader_thread, rout);
    pthread_create(&terr, NULL, reader_thread, rerr);

    int wstatus;
    waitpid(pid, &wstatus, 0);
    pthread_join(tout, NULL);
    pthread_join(terr, NULL);

    flush_cracked(); /* final flush */

    pthread_mutex_lock(&g_mutex);
    g_pid   = -1;
    g_stage = 3;
    if (g_pct < 100.0 && WIFEXITED(wstatus) && WEXITSTATUS(wstatus) <= 1)
        g_pct = 100.0;
    pthread_mutex_unlock(&g_mutex);

    if (!WIFEXITED(wstatus)) return 1;
    int rc2 = WEXITSTATUS(wstatus);
    /* hashcat: 0=found, 1=not found/exhausted, 2+=error.
     * Both 0 and 1 are "normal completion" from lc7's perspective. */
    return (rc2 <= 1) ? 0 : rc2;
}

/* ============================================================
 * PUBLIC API
 * ============================================================ */

/* ABI guard used by CLC7JTRDLL loader */
size_t jtrdll_abi_hooks_struct_size(void)
{
    return sizeof(struct JTRDLL_HOOKS);
}

int jtrdll_main(int argc, char **argv, struct JTRDLL_HOOKS *hooks)
{
    /* Debug: log all calls to help diagnose issues */
    {
        FILE *dbg = fopen("/tmp/jtrdll_debug.log", "a");
        if (dbg) {
            fprintf(dbg, "jtrdll_main argc=%d\n", argc);
            for (int di = 0; di < argc; di++)
                fprintf(dbg, "  argv[%d]='%s'\n", di, argv[di] ? argv[di] : "(null)");
            fclose(dbg);
        }
    }

    /* ---- Self-test mode ---- */
    if (has_arg(argc, argv, "--test=0")) {
        const char *hashcat_bin = resolve_hashcat_bin();
        FILE *dbg = fopen("/tmp/jtrdll_debug.log", "a");
        if (dbg) {
            int accessible = access(hashcat_bin, X_OK);
            fprintf(dbg, "  SELFTEST: hashcat_bin='%s' access=%d\n", hashcat_bin, accessible);
            fclose(dbg);
        }
        if (strcmp(hashcat_bin, "hashcat") != 0 && access(hashcat_bin, X_OK) != 0) {
            if (hooks && hooks->stderr_hook)
                hooks->stderr_hook(hooks->ctx,
                    "hashcat not found (checked /opt/homebrew/bin, /usr/local/bin, PATH)");
            return 1;
        }
        if (hooks && hooks->stdout_hook)
            hooks->stdout_hook(hooks->ctx, "Self test passed.");
        return 0;
    }

    /* ---- Parse JtR arguments ---- */
    const char *jtr_format  = find_arg(argc, argv, "--format");
    const char *jtr_potfile = find_arg(argc, argv, "--pot");
    const char *jtr_wordlist= find_arg(argc, argv, "--wordlist");
    const char *jtr_rules   = find_arg(argc, argv, "--rules");
    const char *jtr_mask    = find_arg(argc, argv, "--mask");
    const char *jtr_incr    = find_arg(argc, argv, "--incremental");
    const char *jtr_minlen  = find_arg(argc, argv, "--min-length");
    const char *jtr_maxlen  = find_arg(argc, argv, "--max-length");
    int         jtr_single  = has_arg(argc, argv, "--single");

    /* Multi-node: LC7 splits work across N nodes for parallel CPU/GPU cracking.
     * We have only one GPU, so running N hashcat instances simultaneously causes
     * GPU contention and file conflicts. Only node 1 runs hashcat; others skip.
     * The full keyspace is covered by node 1. */
    int jtr_node = 1, jtr_total_nodes = 1;
    const char *jtr_node_str = find_arg(argc, argv, "--node");
    if (jtr_node_str) {
        sscanf(jtr_node_str, "%d/%d", &jtr_node, &jtr_total_nodes);
    }
    if (jtr_total_nodes > 1 && jtr_node != 1) {
        return 0;
    }

    /* last positional arg = hash file */
    const char *jtr_hashfile = NULL;
    for (int i = argc - 1; i > 0; i--)
        if (argv[i][0] != '-') { jtr_hashfile = argv[i]; break; }

    int hc_mode = -1;
    if (jtr_format) hc_mode = jtr_format_to_hc_mode(jtr_format);
    if (hc_mode < 0) {
        char msg[256];
        snprintf(msg, sizeof(msg), "Unsupported hash format: %s",
                 jtr_format ? jtr_format : "(none)");
        if (hooks && hooks->stderr_hook) hooks->stderr_hook(hooks->ctx, msg);
        return 1;
    }

    /* Save potfile path for cracked-flush */
    if (jtr_potfile) strncpy(s_jtr_potfile, jtr_potfile, PATH_MAX - 1);

    /* ---- Convert hash file ---- */
    s_tmpcount = 0;
    const char *hc_hashfile = make_tmpfile("_hashes.txt");
    const char *hc_potfile  = make_tmpfile("_hcpot.txt");

    strncpy(s_hc_potfile_read, hc_potfile, PATH_MAX - 1);
    s_hc_pot_offset = 0;

    if (jtr_hashfile) {
        int nh = convert_hashfile(jtr_hashfile, hc_hashfile, hc_mode);
        if (nh <= 0) {
            if (hooks && hooks->stdout_hook)
                hooks->stdout_hook(hooks->ctx, "No hashes to crack.");
            cleanup_tmpfiles();
            return 0;
        }
    } else {
        if (hooks && hooks->stderr_hook)
            hooks->stderr_hook(hooks->ctx, "No hash file specified.");
        cleanup_tmpfiles();
        return 1;
    }

    /* ---- Build hashcat command ---- */
    hcarg_reset();
    hcarg_add(resolve_hashcat_bin());
    hcarg_addf("-m%d", hc_mode);
    /* Unique session name avoids conflicts with leftover .outfiles from crashes */
    hcarg_addf("--session=lc7_%d_%d", (int)getpid(), s_tmpcount);
    /* potfile stores hash:plain — read it to convert to JtR format */
    hcarg_addf("--potfile-path=%s", hc_potfile);
    hcarg_add("--status");
    hcarg_add("--status-timer=1");
    /* Apple hashcat uses OpenCL stack for GPU compute; do not disable it. */
    hcarg_add("--optimized-kernel-enable");
    hcarg_add("--workload-profile=4");
    hcarg_add("--force");               /* skip warnings */

    /* ---- Attack mode ---- */
    if (jtr_single) {
        /* User-Info mode: generate wordlist from usernames */
        const char *wlpath = make_tmpfile("_single.txt");
        if (create_single_wordlist(jtr_hashfile, wlpath) <= 0) {
            cleanup_tmpfiles();
            return 0;
        }
        hcarg_add("-a0");
        hcarg_add(hc_hashfile);
        hcarg_add(wlpath);
        /* Keep previous behavior: User Info uses best64 unless LC7 explicitly passes another rule */
        if (jtr_rules && jtr_rules[0]) {
            add_hashcat_rule_if_any(jtr_rules);
        } else {
            char rules_path[PATH_MAX];
            snprintf(rules_path, PATH_MAX, "%s/best64.rule", HASHCAT_RULES);
            if (access(rules_path, R_OK) == 0)
                hcarg_addf("-r%s", rules_path);
        }

    } else if (jtr_wordlist) {
        /* Wordlist / Dictionary mode */
        hcarg_add("-a0");
        hcarg_add(hc_hashfile);
        hcarg_add(jtr_wordlist);
        add_hashcat_rule_if_any(jtr_rules);

    } else if (jtr_mask) {
        /* Mask / Brute-force mode */
        hcarg_add("-a3");
        hcarg_add(hc_hashfile);

        int minlen = jtr_minlen ? atoi(jtr_minlen) : 0;
        int maxlen = jtr_maxlen ? atoi(jtr_maxlen) : 0;
        if (minlen < 1) minlen = 1; /* hashcat requires increment-min >= 1 */

        /* JtR preset masks may be bracket charsets like "[abc123 ]".
         * Hashcat needs these as custom charsets (-1 ...) referenced by ?1. */
        const char *mask_atom = jtr_mask;
        char custom_charset[256] = {0};
        size_t mlen = strlen(jtr_mask);
        if (mlen >= 2 && jtr_mask[0] == '[' && jtr_mask[mlen - 1] == ']') {
            size_t clen = mlen - 2;
            if (clen >= sizeof(custom_charset)) clen = sizeof(custom_charset) - 1;
            memcpy(custom_charset, jtr_mask + 1, clen);
            custom_charset[clen] = '\0';
            if (custom_charset[0]) {
                hcarg_addf("-1%s", custom_charset);
                mask_atom = "?1";
            }
        }

        char full_mask[256] = {0};

        if (maxlen > 0) {
            /* Variable length: repeat single-element mask up to maxlen */
            int elem_len = (mask_atom[0] == '?' && mask_atom[1]) ? 2 : 1;
            int nmask = 0;
            for (int i = 0; mask_atom[i]; ) {
                if (mask_atom[i] == '?' && mask_atom[i+1]) { nmask++; i += 2; }
                else { nmask++; i++; }
            }
            if (nmask == 1) {
                /* Single element: repeat maxlen times */
                int out = 0;
                for (int j = 0; j < maxlen && out + elem_len < 254; j++) {
                    strncat(full_mask, mask_atom, elem_len);
                    out += elem_len;
                }
            } else {
                strncpy(full_mask, mask_atom, 255);
            }
            hcarg_add("--increment");
            hcarg_addf("--increment-min=%d", minlen);
            hcarg_addf("--increment-max=%d", maxlen);
        } else {
            /* Fixed length: mask already fully specified */
            strncpy(full_mask, mask_atom, 255);
        }
        hcarg_add(full_mask);

    } else if (jtr_incr) {
        /* Incremental / charset mode: use ?a as fallback */
        int minlen = jtr_minlen ? atoi(jtr_minlen) : 1;
        int maxlen = jtr_maxlen ? atoi(jtr_maxlen) : 8;
        if (minlen < 1) minlen = 1; /* hashcat requires increment-min >= 1 */
        if (maxlen < minlen) maxlen = minlen;
        if (maxlen > 12) maxlen = 12; /* reasonable cap */

        hcarg_add("-a3");
        hcarg_add(hc_hashfile);

        char mask[64] = {0};
        for (int j = 0; j < maxlen && strlen(mask) < 60; j++)
            strcat(mask, "?a");
        hcarg_add(mask);
        hcarg_add("--increment");
        hcarg_addf("--increment-min=%d", minlen);
        hcarg_addf("--increment-max=%d", maxlen);

    } else {
        /* No mode recognized */
        if (hooks && hooks->stderr_hook)
            hooks->stderr_hook(hooks->ctx, "No attack mode recognized.");
        cleanup_tmpfiles();
        return 1;
    }

    /* ---- Reset status ---- */
    pthread_mutex_lock(&g_mutex);
    g_stage      = 1;
    g_pct        = 0.0;
    g_elapsed    = 0.0;
    g_eta        = 0.0;
    g_guesses    = 0;
    g_cands      = 0;
    g_speed      = 0.0;
    g_start_time = 0;
    g_word1[0]   = '\0';
    g_word2[0]   = '\0';
    pthread_mutex_unlock(&g_mutex);

    /* ---- Run hashcat ---- */
    int rc = run_hashcat(hooks);

    cleanup_tmpfiles();
    return rc;
}

/* ------------------------------------------------------------ */
void jtrdll_abort(int timeout)
{
    pthread_mutex_lock(&g_mutex);
    pid_t pid = g_pid;
    pthread_mutex_unlock(&g_mutex);

    if (pid > 0) {
        /* SIGINT = graceful (saves pot/session), SIGTERM = hard stop */
        kill(pid, timeout ? SIGTERM : SIGINT);
    }
}

/* ------------------------------------------------------------ */
void jtrdll_get_status(struct JTRDLL_STATUS *st)
{
    pthread_mutex_lock(&g_mutex);
    memset(st, 0, sizeof(*st));
    st->stage                = g_stage;
    st->percent              = g_pct;
    /* Use wall-clock elapsed so progress bars work even if hashcat
       does not emit Time.Running lines (it emits Time.Started instead). */
    if (g_stage == 2 && g_start_time > 0)
        st->time = (double)(time(NULL) - g_start_time);
    else
        st->time = g_elapsed;
    st->eta                  = g_eta;
    st->guess_count          = g_guesses;
    st->candidates           = g_cands;
    st->guesses_per_second   = g_speed;
    st->candidates_per_second= g_speed;
    st->crypts_per_second    = g_speed;
    strncpy(st->word1, g_word1, sizeof(st->word1) - 1);
    strncpy(st->word2, g_word2, sizeof(st->word2) - 1);
    pthread_mutex_unlock(&g_mutex);
}

/* ------------------------------------------------------------ */
int jtrdll_get_charset_info(const char *path,
                             unsigned char *charmin, unsigned char *charmax,
                             unsigned char *len,     unsigned char *count,
                             unsigned char  allchars[256])
{
    /* Try to parse JtR .chr file header (V1 format):
     * bytes 0-7: magic/version, byte 8: char count, byte 9: max length */
    FILE *f = fopen(path, "rb");
    if (f) {
        unsigned char hdr[12];
        if (fread(hdr, 1, 12, f) == 12) {
            /* JtR V1: magic "\x12\x34\x56\x78", then "1.0\0" or similar */
            if (hdr[0] == 0x12 && hdr[1] == 0x34 && hdr[2] == 0x56 && hdr[3] == 0x78) {
                *count = hdr[8] ? hdr[8] : 95;
                *len   = hdr[9] ? hdr[9] : 8;
                fclose(f);
                /* We don't decode the full charset table; return all printable */
                goto fill_printable;
            }
        }
        fclose(f);
    }

fill_printable:
    /* Default: all printable ASCII */
    *charmin = 0x20;
    *charmax = 0x7e;
    if (!*len)   *len   = 8;
    if (!*count) *count = 95;
    memset(allchars, 0, 256);
    for (int i = 0x20; i <= 0x7e; i++) allchars[i] = 1;
    return 0;
}

/* ------------------------------------------------------------ */
void jtrdll_cleanup(void)
{
    jtrdll_abort(1);
    pthread_mutex_lock(&g_mutex);
    g_stage = 0;
    g_pid   = -1;
    pthread_mutex_unlock(&g_mutex);
    s_hc_potfile_read[0] = '\0';
    s_jtr_potfile[0]     = '\0';
    s_hc_pot_offset = 0;
}

/* ------------------------------------------------------------ */
void jtrdll_preflight(int argc, char **argv, struct JTRDLL_HOOKS *hooks,
                      struct JTRDLL_PREFLIGHT *pf)
{
    memset(pf, 0, sizeof(*pf));

    if (has_arg(argc, argv, "--test=0")) {
        pf->valid = 1;
        return;
    }

    const char *jtr_format  = find_arg(argc, argv, "--format");
    const char *jtr_wordlist= find_arg(argc, argv, "--wordlist");
    const char *jtr_mask    = find_arg(argc, argv, "--mask");
    const char *jtr_incr    = find_arg(argc, argv, "--incremental");
    const char *jtr_minlen  = find_arg(argc, argv, "--min-length");
    const char *jtr_maxlen  = find_arg(argc, argv, "--max-length");
    int         jtr_single  = has_arg(argc, argv, "--single");

    const char *jtr_hashfile = NULL;
    for (int i = argc - 1; i > 0; i--)
        if (argv[i][0] != '-') { jtr_hashfile = argv[i]; break; }

    int hc_mode = -1;
    if (jtr_format) hc_mode = jtr_format_to_hc_mode(jtr_format);
    if (hc_mode < 0) return;

    /* Count hashes */
    int nhashes = 0;
    if (jtr_hashfile) {
        FILE *f = fopen(jtr_hashfile, "r");
        if (f) {
            char line[2048];
            while (fgets(line, sizeof(line), f))
                if (strchr(line, ':')) nhashes++;
            fclose(f);
        }
    }
    pf->salt_count = nhashes;

    if (jtr_wordlist) {
        /* count wordlist lines */
        FILE *f = fopen(jtr_wordlist, "r");
        if (f) {
            uint64_t lines = 0;
            char line[1024];
            while (fgets(line, sizeof(line), f)) lines++;
            fclose(f);
            pf->wordlist_rule_count = lines;
        }
    } else if (jtr_mask || jtr_incr) {
        int maxlen = jtr_maxlen ? atoi(jtr_maxlen) : 8;
        /* Rough estimate: 95^maxlen for all-printable */
        uint64_t est = 1;
        for (int i = 0; i < maxlen; i++) {
            if (est > (uint64_t)1e18 / 95) { est = (uint64_t)1e18; break; }
            est *= 95;
        }
        pf->mask_candidates       = est;
        pf->incremental_candidates = est;
    } else if (jtr_single) {
        pf->wordlist_rule_count = (uint64_t)nhashes * 50;
    }

    pf->valid = 1;
}

/* ------------------------------------------------------------ */
void jtrdll_set_extra_opencl_kernel_args(const char *args)
{
    (void)args; /* no-op: hashcat handles GPU internally */
}
