# Security Policy

## Supported versions

Security fixes are applied to the latest code on the repository's default
branch. Before a stable release exists, older snapshots are not maintained.

## Report a vulnerability

Do not open a public issue for a vulnerability that could expose console data,
allow unintended privileged operations, bypass plugin capabilities, corrupt
memory through malformed IPC, or execute untrusted code.

Use GitHub's **Security** tab and select **Report a vulnerability** to submit a
private report. Include:

- affected commit or version;
- affected public API, tool, or wire message;
- reproduction steps or a minimal proof of concept;
- expected impact and required privileges;
- suggested mitigation, if known.

Maintainers will acknowledge a complete report when it is reviewed, coordinate
a fix privately when practical, and credit the reporter unless anonymity is
requested. Please do not disclose the issue publicly before a fix or coordinated
disclosure decision.

Ordinary crashes, build failures, and documentation mistakes should use the
public [bug report form](https://github.com/aydencharles/onionHEN-plugin-sdk/issues/new?template=bug_report.yml).

## Project scope

This repository is unofficial PS5 homebrew and is provided without warranty.
Reports about Sony services, PlayStation Network, exploit hosts, unrelated
payloads, or vulnerabilities in the PS5 platform itself are outside this
project's disclosure scope.

