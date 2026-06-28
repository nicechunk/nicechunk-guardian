# Security Policy

nicechunk-guardian is a focused split from the NiceChunk working tree. Public split repositories must not contain private keys, wallet files, deployment-only scripts, machine-specific configuration, production tokens, server addresses, local debug material, or generated build artifacts.

## Reporting a Vulnerability

Do not open public issues for suspected leaks or exploitable vulnerabilities. Send a private report to the project owner with the affected repository, commit hash, file paths, and a concise reproduction.

## Repository Rules

- Keep .auth/, .deploy/, .gh-config/, .ssh/, debug/, deploy/, dist/, build/, target/, and Guardian/build/ out of GitHub.
- Keep server sync scripts and deployment scripts out of public repositories.
- Use nicechunk <293527782+nicechunk@users.noreply.github.com> for project sync commits.
- Run the split audit before pushing generated repository content.
- Use the public NiceChunk threat model for high-risk trust boundary or protected asset changes.
