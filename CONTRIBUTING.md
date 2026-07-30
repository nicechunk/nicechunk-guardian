# Contributing to nicechunk-guardian

NiceChunk Guardian realtime service.

This repository is generated from the main NiceChunk working tree. Make code changes in the main tree, run the relevant validation, then regenerate the split repository with:

~~~bash
node scripts/split-github-repos.mjs --repo nicechunk-guardian
~~~

## Pull Request Expectations

- Keep changes scoped to this repository's domain.
- Explain what changed and why.
- List validation commands.
- Do not include private keys, tokens, server IPs, deployment scripts, or machine-specific files.
- Keep user-facing copy behind i18n where the surface already uses locales.

## Commit Identity and Approval Evidence

Use an authorized contributor or team identity that follows the project's current attribution policy. Do not fabricate an identity or copy identity metadata from an unrelated commit.

For this repository, authorization is concrete: use your own identity, an organization-managed bot identity, or a team identity that a repository maintainer explicitly assigned for this exact change. The reserved `nicechunk <293527782+nicechunk@users.noreply.github.com>` team identity is not a default. If no authorized identity is available, stop before committing and ask a maintainer to select one. GitHub maps commit authors by email, but that mapping does not establish authorization.

Git author and committer fields record attribution only. They do not prove review, approval, wallet control, branch-protection enforcement, or deployment authorization. Cite the actual pull request review, repository ruleset result, release record, or other control evidence when such approval is required; do not claim a control is enforced unless evidence exists for the exact revision.
