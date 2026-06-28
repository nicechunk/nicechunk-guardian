# nicechunk-guardian Changelog

All notable public changes for this split repository should be recorded here.

This repository is generated from the main NiceChunk working tree. Make source changes in the main tree, regenerate the split repositories, then commit and push this split separately.

## Release Note Rules

- Use English for public GitHub changelog entries and release notes.
- Reference the main-tree commit and this split repository commit.
- List validation commands that were run for this split or its source surface.
- Do not include private server addresses, credentials, deployment-only scripts, raw production logs, wallet secrets, or unreleased private infrastructure details.
- Mark manual release gates as deferred unless evidence exists for the exact commit under review.

## Unreleased

### Split Scope

NiceChunk Guardian realtime service.

### Current Status

- Generated from the main NiceChunk working tree.
- Apache-2.0 license metadata is included in `LICENSE`, `NOTICE`, and `docs/license-status.md`.
- CI workflow publication is documented in `docs/ci-workflow-spec.md` and remains pending credentials with `workflow` scope.

### Source Anchors

Fill these fields before publishing a release note:

- Main-tree commit: `<main NiceChunk working tree commit>`
- Split repository commit: `<split repository commit>`

### Release Evidence Checklist

Before publishing a release note for this split, cite:

~~~bash
npm run validate:repo
npm run audit:split-remotes
npm run audit:maturity
~~~

If this split contains `package.json`, also run the relevant local scripts such as `npm run audit:deps`, `npm run audit:licenses`, `npm run build`, or the split-specific `validate:release` script when present.
