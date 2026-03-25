#pragma once

#include <QString>

/*
 * Build a John wordlist from an LC7 JtR hash file (User Info / single-crack input).
 * Lines look like: username:$NT$hex:::userinfo::
 * Mirrors the former macOS hashcat-wrapper candidate set (suffix mutations + token splits).
 */
bool lc7WriteUserInfoWordlist(const QString &jtrHashFilePath, const QString &outWordlistPath, QString *errorOut);
