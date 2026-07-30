# Security Policy

nicechunk-guardian is a focused split from the NiceChunk working tree. Public split repositories must not contain private keys, wallet files, deployment-only scripts, machine-specific configuration, production tokens, server addresses, local debug material, or generated build artifacts.

## Reporting a Vulnerability

Do not put a suspected vulnerability, secret, affected component, impact, reproduction, or exploit detail in a public issue.

This policy does not claim that GitHub Private Vulnerability Reporting is enabled. Verify that GitHub displays the private form at report time.

1. Try the [GitHub private vulnerability report form](https://github.com/nicechunk/nicechunk-guardian/security/advisories/new). Use it only when GitHub displays a private advisory form for this repository.
2. If the form is unavailable, open the [security-coordination issue](https://github.com/nicechunk/nicechunk-guardian/issues/new?title=%5BSecurity%20coordination%5D%20Enable%20private%20vulnerability%20reporting&body=Please%20enable%20GitHub%20Private%20Vulnerability%20Reporting%20for%20a%20confidential%20report.%20No%20vulnerability%20details%20are%20included%20here.) with no vulnerability details. Ask the maintainer to enable GitHub Private Vulnerability Reporting; do not identify the affected component or impact.
3. After the repository reports that private reporting is enabled, submit the affected repository, commit hash, file paths, impact, and concise reproduction through the private form. Do not send secrets themselves; describe and rotate them.

This fallback is public coordination only, not a disclosure channel. Wait for a verified private form before sharing technical details.

## Repository Rules

- Keep .auth/, .deploy/, .gh-config/, .ssh/, debug/, deploy/, dist/, build/, target/, and Guardian/build/ out of GitHub.
- Keep server sync scripts and deployment scripts out of public repositories.
- A commit identity is authorized only when it is the contributor's own identity, an organization-managed bot identity, or a team identity that a repository maintainer explicitly assigned for that exact change.
- The reserved team identity `nicechunk <293527782+nicechunk@users.noreply.github.com>` is not a default; use it only when a repository owner explicitly assigns it for the exact commit.
- If none of those conditions applies, stop before committing and ask a repository maintainer to choose the author identity. Never copy an identity from an unrelated commit or attribute work to another person without permission.
- Treat commit author and committer metadata as attribution only, never as proof of review, approval, wallet control, or release authorization.
- Run the split audit before pushing generated repository content.
- Use the public NiceChunk threat model for high-risk trust boundary or protected asset changes.
