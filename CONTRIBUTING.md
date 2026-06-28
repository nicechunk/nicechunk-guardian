# Contributing to nicechunk-guardian

NiceChunk Guardian realtime service.

This repository is generated from the main NiceChunk working tree. Make code changes in the main tree, run the relevant validation, then regenerate the split repository with:

~~~bash
node scripts/split-github-repos.mjs
~~~

## Pull Request Expectations

- Keep changes scoped to this repository's domain.
- Explain what changed and why.
- List validation commands.
- Do not include private keys, tokens, server IPs, deployment scripts, or machine-specific files.
- Keep user-facing copy behind i18n where the surface already uses locales.

## Commit Identity

NiceChunk sync commits should use:

~~~text
nicechunk <293527782+nicechunk@users.noreply.github.com>
~~~

GitHub maps commit authors by email, so avoid personal or agent-derived emails for project sync commits.
