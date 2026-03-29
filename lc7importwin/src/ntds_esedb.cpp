// ntds_esedb.cpp — macOS/Linux NTDS.DIT parser using libesedb (LGPL)
// Replaces the Windows ESENT (Jet) implementation in ntds.cpp on non-Windows targets.
#if defined(__APPLE__) || defined(__linux__)

// Include libesedb BEFORE ntds.h so real typedefs take precedence over void* aliases
#include<libesedb.h>
#include<stdafx.h>
#include"ntds.h"
#include"ntreg.h"
#include"crypt.h"
#include"slow_des.h"
#include<memory>
#include<map>
#include<string>
#include<vector>

// Cast helpers: ntds.h uses void* for libesedb handles; cast to real types here
#define AS_FILE(p)   static_cast<libesedb_file_t*>(p)
#define AS_TABLE(p)  static_cast<libesedb_table_t*>(p)
#define AS_RECORD(p) static_cast<libesedb_record_t*>(p)

// ─── libesedb helper shims ───────────────────────────────────────────────────
// All helpers take void* (= libesedb_record_t*) and cast internally so that
// calling code can use the void* signatures from ntds.h.

static std::vector<uint8_t> esedb_get_binary(void *vrec, int col)
{
    libesedb_record_t *rec = AS_RECORD(vrec);
    libesedb_error_t *err = nullptr;
    size_t sz = 0;
    if (libesedb_record_get_value_binary_data_size(rec, col, &sz, &err) != 1 || sz == 0)
    { libesedb_error_free(&err); return {}; }
    std::vector<uint8_t> buf(sz);
    if (libesedb_record_get_value_binary_data(rec, col, buf.data(), sz, &err) != 1)
    { libesedb_error_free(&err); return {}; }
    return buf;
}

static std::wstring esedb_get_wstring(void *vrec, int col)
{
    libesedb_record_t *rec = AS_RECORD(vrec);
    libesedb_error_t *err = nullptr;
    size_t sz = 0;
    if (libesedb_record_get_value_utf16_string_size(rec, col, &sz, &err) != 1 || sz == 0)
    { libesedb_error_free(&err); return {}; }
    std::vector<uint16_t> buf(sz);
    if (libesedb_record_get_value_utf16_string(rec, col, buf.data(), sz, &err) != 1)
    { libesedb_error_free(&err); return {}; }
    std::wstring ws;
    for (size_t i = 0; i < sz && buf[i] != 0; ++i)
        ws += static_cast<wchar_t>(buf[i]);
    return ws;
}

static bool esedb_get_u32(void *vrec, int col, uint32_t &out)
{
    libesedb_record_t *rec = AS_RECORD(vrec);
    libesedb_error_t *err = nullptr;
    bool ok = (libesedb_record_get_value_32bit(rec, col, &out, &err) == 1);
    libesedb_error_free(&err);
    return ok;
}

static bool esedb_get_u64(void *vrec, int col, uint64_t &out)
{
    libesedb_record_t *rec = AS_RECORD(vrec);
    libesedb_error_t *err = nullptr;
    bool ok = (libesedb_record_get_value_64bit(rec, col, &out, &err) == 1);
    libesedb_error_free(&err);
    return ok;
}

// ─── Column-name → record-index map ─────────────────────────────────────────

using ColMap = std::map<std::string, int>;

// Build a map: column_name → column-index-within-record for 'table'.
// (In libesedb, the column index used in libesedb_record_get_value_*
//  equals the column's position in the table's column list.)
static ColMap BuildColMap(void *vtable)
{
    libesedb_table_t *table = AS_TABLE(vtable);
    ColMap m;
    libesedb_error_t *err = nullptr;
    int ncols = 0;
    if (libesedb_table_get_number_of_columns(table, &ncols, 0, &err) != 1)
    { libesedb_error_free(&err); return m; }
    for (int i = 0; i < ncols; ++i)
    {
        libesedb_column_t *col = nullptr;
        if (libesedb_table_get_column(table, i, &col, 0, &err) != 1)
        { libesedb_error_free(&err); continue; }
        size_t name_size = 0;
        libesedb_column_get_utf8_name_size(col, &name_size, &err);
        if (name_size > 0)
        {
            std::vector<uint8_t> name_buf(name_size);
            libesedb_column_get_utf8_name(col, name_buf.data(), name_size, &err);
            std::string name(reinterpret_cast<const char*>(name_buf.data()));
            m[name] = i;
        }
        libesedb_column_free(&col, nullptr);
    }
    return m;
}

// ─── NTDS class — macOS implementation ───────────────────────────────────────

NTDS::NTDS(bool with_history, bool include_machine_accounts)
{
    m_with_history = with_history;
    m_include_machine_accounts = include_machine_accounts;
    Reset();
}

NTDS::~NTDS()
{
    QString error;
    Close(error);
}

void NTDS::Reset()
{
    m_file  = nullptr;
    m_table = nullptr;
    m_using_recovery    = false;
    m_recovery_tempdir  = "";
    m_ntdsfilename      = "";
    m_dnt_to_domain.clear();
    m_colmap.clear();
}

// Not used on macOS (no log-recovery needed for read-only access), kept for API compat.
bool NTDS::Init(QString &error)
{
    (void)error;
    return true;
}

bool NTDS::Close(QString &error)
{
    (void)error;
    libesedb_error_t *err = nullptr;
    if (m_table) { libesedb_table_t *t = AS_TABLE(m_table); libesedb_table_free(&t, &err); m_table = nullptr; }
    if (m_file)  { libesedb_file_t  *f = AS_FILE(m_file);  libesedb_file_close(f, &err); libesedb_file_free(&f, &err); m_file = nullptr; }
    if (m_using_recovery)
    {
        QDir(m_recovery_tempdir).removeRecursively();
        m_using_recovery = false;
    }
    return true;
}

// Not applicable on macOS — libesedb opens dirty databases fine.
bool NTDS::RecoverDatabase(QString in_ntds, QString &recovered_ntds, QString &error)
{
    (void)error;
    recovered_ntds = in_ntds;
    return true;
}

bool NTDS::OpenDatabase(QString strNTDSPath, QString &error)
{
    libesedb_error_t *err = nullptr;
    m_ntdsfilename = strNTDSPath;

    libesedb_file_t *file_handle = nullptr;
    if (libesedb_file_initialize(&file_handle, &err) != 1)
    {
        error = "libesedb: failed to initialize file handle.";
        libesedb_error_free(&err);
        m_file = nullptr;
        return false;
    }
    // m_file assigned below after open succeeds

    m_file = file_handle; // store as void*

    QByteArray path = m_ntdsfilename.toLocal8Bit();
    if (libesedb_file_open(file_handle, path.constData(), LIBESEDB_OPEN_READ, &err) != 1)
    {
        char msg[256] = {};
        libesedb_error_sprint(err, msg, sizeof(msg));
        error = QString("libesedb: unable to open database: %1").arg(msg);
        libesedb_error_free(&err);
        libesedb_file_free(&file_handle, nullptr);
        m_file = nullptr;
        return false;
    }

    // Open the "datatable" table
    const char *tbl_name = NTDS_TBL_OBJ;
    libesedb_table_t *table_handle = nullptr;
    if (libesedb_file_get_table_by_utf8_name(
            file_handle,
            reinterpret_cast<const uint8_t*>(tbl_name),
            strlen(tbl_name),
            &table_handle,
            &err) != 1)
    {
        error = QString("libesedb: datatable not found in NTDS.DIT. "
                        "Ensure the file was properly extracted with Volume Shadow Copy or ntdsutil.");
        libesedb_error_free(&err);
        libesedb_file_close(file_handle, nullptr);
        libesedb_file_free(&file_handle, nullptr);
        m_file  = nullptr;
        m_table = nullptr;
        return false;
    }
    m_table = table_handle; // store as void*

    // Build column map once
    m_colmap = BuildColMap(m_table);

    return true;
}

bool NTDS::CloseDatabase(QString &error)
{
    return Close(error);
}

// ─── Helper: get column index from name, or -1 ───────────────────────────────

static int ColIdx(const ColMap &m, const std::string &name)
{
    auto it = m.find(name);
    return (it != m.end()) ? it->second : -1;
}

// ─── Pass 1: collect PEK and domain→DNT mapping ──────────────────────────────

int NTDS::NTLM_ParsePEKRecord(void *rec, CRYPTED_DATA **pek_ciphered, size_t *len_pek_ciphered)
{
    int col = ColIdx(m_colmap, ATT_PEK);
    if (col < 0) return NTDS_BAD_RECORD;

    auto data = esedb_get_binary(rec, col);
    if (data.empty()) return NTDS_BAD_RECORD;

    size_t sz = data.size();
    if (sz < 56) return NTDS_BAD_RECORD;   // minimum: header + one entry

    auto *cd = reinterpret_cast<CRYPTED_DATA*>(data.data());
    if (cd->dwAlgorithmId != 2 && cd->dwAlgorithmId != 3)
        return NTDS_BAD_RECORD;

    *pek_ciphered = (CRYPTED_DATA*)malloc(sz);
    if (!*pek_ciphered) return NTDS_MEM_ERROR;
    memcpy(*pek_ciphered, data.data(), sz);
    *len_pek_ciphered = sz;
    return NTDS_SUCCESS;
}

int NTDS::ParseDomainRecords(void *rec)
{
    int col_dnt  = ColIdx(m_colmap, "DNT_col");
    int col_dom  = ColIdx(m_colmap, ATT_DOMAIN);
    if (col_dnt < 0 || col_dom < 0) return NTDS_BAD_RECORD;

    uint32_t dnt = 0;
    if (!esedb_get_u32(rec, col_dnt, dnt)) return NTDS_BAD_RECORD;

    auto dom = esedb_get_wstring(rec, col_dom);
    if (!dom.empty())
        m_dnt_to_domain[dnt] = dom;

    return NTDS_SUCCESS;
}

// ─── Pass 2: parse individual SAM user records ────────────────────────────────

int NTDS::NTLM_ParseSAMRecord(void *rec, LDAPAccountInfo ldapEntry)
{
    // SAMAccountType (must be user or machine)
    int col_type = ColIdx(m_colmap, ATT_SAM_ACCOUNT_TYPE);
    if (col_type < 0) return NTDS_BAD_RECORD;
    uint32_t sam_type = 0;
    if (!esedb_get_u32(rec, col_type, sam_type)) return NTDS_BAD_RECORD;

    if (sam_type != SAM_USER_OBJECT &&
        sam_type != SAM_MACHINE_ACCOUNT &&
        sam_type != SAM_TRUST_ACCOUNT)
        return NTDS_BAD_RECORD;

    if (!m_include_machine_accounts && sam_type != SAM_USER_OBJECT)
        return NTDS_BAD_RECORD;

    ldapEntry->dwSAMAccountType = sam_type;

    // SAMAccountName
    int col_name = ColIdx(m_colmap, ATT_SAM_ACCOUNT_NAME);
    if (col_name < 0) return NTDS_BAD_RECORD;
    auto acct_name = esedb_get_wstring(rec, col_name);
    if (acct_name.empty()) return NTDS_BAD_RECORD;
    ldapEntry->szSAMAccountName = acct_name;

    // UserAccountControl
    int col_uac = ColIdx(m_colmap, ATT_USER_ACCOUNT_CONTROL);
    if (col_uac >= 0) {
        uint32_t uac = 0;
        if (esedb_get_u32(rec, col_uac, uac)) ldapEntry->dwAccountControl = uac;
    }

    // Description
    int col_desc = ColIdx(m_colmap, ATT_DESCRIPTION);
    if (col_desc >= 0) ldapEntry->szDescription = esedb_get_wstring(rec, col_desc);

    // Home directory
    int col_home = ColIdx(m_colmap, ATT_HOME_DIRECTORY);
    if (col_home >= 0) ldapEntry->szHomeDirectory = esedb_get_wstring(rec, col_home);

    // Surname
    int col_sn = ColIdx(m_colmap, ATT_SURNAME);
    if (col_sn >= 0) ldapEntry->szSurname = esedb_get_wstring(rec, col_sn);

    // Given name
    int col_gn = ColIdx(m_colmap, ATT_GIVEN_NAME);
    if (col_gn >= 0) ldapEntry->szGivenName = esedb_get_wstring(rec, col_gn);

    // Org unit
    int col_ou = ColIdx(m_colmap, ATT_ORG_UNIT);
    if (col_ou >= 0) ldapEntry->szOrgUnit = esedb_get_wstring(rec, col_ou);

    // Password last set (FILETIME)
    int col_ts = ColIdx(m_colmap, ATT_PASSWORD_LAST_SET_INDEX);
    if (col_ts >= 0) {
        uint64_t ft = 0;
        if (esedb_get_u64(rec, col_ts, ft)) {
            ldapEntry->ftLastChangedTime.dwLowDateTime  = (DWORD)(ft & 0xFFFFFFFF);
            ldapEntry->ftLastChangedTime.dwHighDateTime = (DWORD)(ft >> 32);
        }
    }

    // LM hash (encrypted blob)
    int col_lm = ColIdx(m_colmap, ATT_LM_HASH);
    if (col_lm >= 0) {
        auto data = esedb_get_binary(rec, col_lm);
        if (data.size() >= WIN_NTLM_HASH_SIZE + 24) {
            ldapEntry->LM_hash_ciphered = (CRYPTED_DATA*)malloc(data.size());
            if (!ldapEntry->LM_hash_ciphered) return NTDS_MEM_ERROR;
            memcpy(ldapEntry->LM_hash_ciphered, data.data(), data.size());
            ldapEntry->NTLM_hash.has_lm_hash = true;
        }
    }

    // NT hash (encrypted blob)
    int col_nt = ColIdx(m_colmap, ATT_NT_HASH);
    if (col_nt >= 0) {
        auto data = esedb_get_binary(rec, col_nt);
        if (data.size() >= WIN_NTLM_HASH_SIZE + 24) {
            ldapEntry->NT_hash_ciphered = (CRYPTED_DATA*)malloc(data.size());
            if (!ldapEntry->NT_hash_ciphered) return NTDS_MEM_ERROR;
            memcpy(ldapEntry->NT_hash_ciphered, data.data(), data.size());
            ldapEntry->NTLM_hash.has_nt_hash = true;
        }
    }

    // Hash history (optional)
    if (m_with_history) {
        int col_lmh = ColIdx(m_colmap, ATT_LM_HASH_HISTORY);
        if (col_lmh >= 0) {
            auto data = esedb_get_binary(rec, col_lmh);
            if (!data.empty()) {
                ldapEntry->LM_history_ciphered  = (CRYPTED_DATA*)malloc(data.size());
                ldapEntry->LM_history_deciphered = (LPBYTE)malloc(data.size());
                if (!ldapEntry->LM_history_ciphered || !ldapEntry->LM_history_deciphered)
                    return NTDS_MEM_ERROR;
                memcpy(ldapEntry->LM_history_ciphered, data.data(), data.size());
                ldapEntry->LM_history_ciphered_size = (UINT)data.size();
                ldapEntry->nbHistoryEntriesLM = (UINT)((data.size() - 24) / WIN_NTLM_HASH_SIZE);
            }
        }

        int col_nth = ColIdx(m_colmap, ATT_NT_HASH_HISTORY);
        if (col_nth >= 0) {
            auto data = esedb_get_binary(rec, col_nth);
            if (!data.empty()) {
                ldapEntry->NT_history_ciphered  = (CRYPTED_DATA*)malloc(data.size());
                ldapEntry->NT_history_deciphered = (LPBYTE)malloc(data.size());
                if (!ldapEntry->NT_history_ciphered || !ldapEntry->NT_history_deciphered)
                    return NTDS_MEM_ERROR;
                memcpy(ldapEntry->NT_history_ciphered, data.data(), data.size());
                ldapEntry->NT_history_ciphered_size = (UINT)data.size();
                ldapEntry->nbHistoryEntriesNT = (UINT)((data.size() - 24) / WIN_NTLM_HASH_SIZE);
            }
        }
    }

    // Object SID → RID (last 4 bytes, big-endian)
    int col_sid = ColIdx(m_colmap, ATT_OBJECT_SID);
    if (col_sid >= 0) {
        auto data = esedb_get_binary(rec, col_sid);
        if (data.size() >= 4) {
            uint32_t rid_be;
            memcpy(&rid_be, data.data() + data.size() - 4, 4);
            ldapEntry->rid = BSWAP(rid_be);
        }
    }

    // Domain: walk ANCESTORS_col to find domain from DNT map
    int col_anc = ColIdx(m_colmap, "ANCESTORS_col");
    if (col_anc >= 0) {
        auto data = esedb_get_binary(rec, col_anc);
        if (data.size() >= 4) {
            size_t ndwords = data.size() / 4;
            const uint32_t *pdw = reinterpret_cast<const uint32_t*>(data.data());
            for (size_t i = 0; i < ndwords; ++i) {
                uint32_t pdnt = pdw[i];
                auto it = m_dnt_to_domain.find(pdnt);
                if (it != m_dnt_to_domain.end()) {
                    ldapEntry->szDomain = it->second;
                }
            }
        }
    }

    return NTDS_SUCCESS;
}

// ─── Main parse function: two-pass scan ─────────────────────────────────────

int NTDS::NTLM_ParseDatabase(std::list<LDAPAccountInfo> &ldapAccountInfo,
                             CRYPTED_DATA **pek_ciphered,
                             size_t *len_pek_ciphered,
                             QString &error)
{
    if (!m_table) { error = "NTDS not opened."; return NTDS_API_ERROR; }

    libesedb_table_t *table = AS_TABLE(m_table);
    libesedb_error_t *err = nullptr;
    int nrecs = 0;
    if (libesedb_table_get_number_of_records(table, &nrecs, &err) != 1)
    {
        error = "libesedb: unable to get record count.";
        libesedb_error_free(&err);
        return NTDS_API_ERROR;
    }

    // ── Pass 1: PEK + domain records ─────────────────────────────────────────
    *pek_ciphered     = nullptr;
    *len_pek_ciphered = 0;
    bool got_pek = false;

    for (int i = 0; i < nrecs; ++i)
    {
        libesedb_record_t *rec = nullptr;
        if (libesedb_table_get_record(table, i, &rec, &err) != 1)
        { libesedb_error_free(&err); continue; }

        if (!got_pek)
        {
            if (NTLM_ParsePEKRecord(rec, pek_ciphered, len_pek_ciphered) == NTDS_SUCCESS)
                got_pek = true;
        }
        ParseDomainRecords(rec);
        libesedb_record_free(&rec, nullptr);
    }

    if (!got_pek)
    {
        error = "Unable to decrypt NTDS.DIT: PEK record not found. "
                "Ensure NTDS.DIT was exported with the SYSTEM hive using "
                "ntdsutil or Volume Shadow Copy.";
        return NTDS_BAD_RECORD;
    }

    // ── Pass 2: SAM user records ──────────────────────────────────────────────
    for (int i = 0; i < nrecs; ++i)
    {
        libesedb_record_t *rec = nullptr;
        if (libesedb_table_get_record(table, i, &rec, &err) != 1)
        { libesedb_error_free(&err); continue; }

        LDAPAccountInfo entry = std::make_shared<LDAP_ACCOUNT_INFO>();
        int rc = NTLM_ParseSAMRecord(rec, entry);
        if (rc == NTDS_SUCCESS)
            ldapAccountInfo.push_back(entry);
        else if (rc == NTDS_MEM_ERROR)
        {
            libesedb_record_free(&rec, nullptr);
            error = "Memory error during NTDS parsing.";
            free(*pek_ciphered); *pek_ciphered = nullptr;
            return NTDS_MEM_ERROR;
        }
        libesedb_record_free(&rec, nullptr);
    }

    if (ldapAccountInfo.empty())
        return NTDS_EMPTY_ERROR;

    return NTDS_SUCCESS;
}

// Bitlocker not yet ported to macOS
int NTDS::Bitlocker_ParseDatabase(std::list<BitlockerAccountInfo> &, QString &error)
{
    error = "Bitlocker parsing is not supported on this platform.";
    return NTDS_API_ERROR;
}

#endif // __APPLE__ || __linux__
