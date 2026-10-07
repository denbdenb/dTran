# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |
| < 1.0   | :x:                |

---

## Reporting a Vulnerability

We take the security of **dTran** seriously. If you discover a security vulnerability, please do NOT create a public issue with sensitive details or credentials.

### How to Report
1. **GitHub Security Advisory:** Please submit a report through GitHub's [Private Vulnerability Reporting](https://github.com/denb/dTran/security/advisories/new) if enabled on the repository.
2. If private reporting is not available, open a draft issue marked with `[SECURITY]` without disclosing exploit payloads or personal data.

### Scope & Expectations
- **dTran** operates as a local Windows desktop client.
- The client does not host remote listener services, databases, or cloud accounts.
- Translation requests are dispatched directly from your local machine to Google and Yandex over HTTPS.
- Local configuration and translation history are stored strictly on your local computer (`%LOCALAPPDATA%`).
