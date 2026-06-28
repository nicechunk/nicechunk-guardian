# nicechunk-guardian CI Workflow Status

nicechunk-guardian does not currently publish a GitHub Actions workflow file.

This split repository is generated from the main NiceChunk working tree. The intended CI design is maintained in the main repository at `docs/ci-workflow-spec.md`. Workflow publication is pending credentials with `workflow` scope.

## Local Equivalent

Reviewers should run the checks that exist in this split repository's `package.json`, if present. Common examples are:

~~~bash
npm ci
npm run audit:deps
npm run build
~~~

Some split repositories are documentation, assets, or C++ service surfaces without npm build scripts. In those cases, use `SECURITY.md`, `CONTRIBUTING.md`, repository health files, and the main-tree release validation as the source of CI evidence.

## Activation Rule

Do not add `.github/workflows/*` to this split repository until the project owner provides a credential with `workflow` scope and the workflow is synced with the main NiceChunk CI specification.
