# CODEX_RULES

## Required Reading Order

Before starting implementation work, read these documents first:

1. `/Users/simjuheun/Developer/JUCE_prac/PROJECT_CONTEXT.md`
2. `/Users/simjuheun/Developer/JUCE_prac/AGENTS.md`
3. `/Users/simjuheun/Developer/JUCE_prac/API_CONTRACT.md`

## Path and Structure Rules

- Do not change directory paths arbitrarily.
- Do not move files or restructure projects unless explicitly requested.
- Keep `VoltaAgentPlugin` and `mixing-extension` as separate projects.

## Change Planning Rules

- Before modifying code, summarize the change plan first.
- Prefer small, targeted edits over wide-scope refactors.
- Avoid unnecessary large-scale refactoring.

## Audio Thread Safety Rules

- Never perform network calls inside `processBlock()`.
- Do not run heavy, blocking, or non-real-time-safe work on the audio thread.
- Keep server communication asynchronous and outside the real-time processing path.

## API Contract Rules

- If any API field name changes, update `/Users/simjuheun/Developer/JUCE_prac/API_CONTRACT.md` in the same task.
- Keep request and response schemas aligned between plugin and server.
- Treat `API_CONTRACT.md` as the shared contract between separated projects.

## Build and Verification Rules

- Always suggest build commands after making implementation changes.
- Always suggest test or verification commands after making implementation changes.
- If no automated test exists, provide a manual verification path.

## Documentation and Logging Rules

- After completing changes, leave a concise summary in `/Users/simjuheun/Developer/JUCE_prac/WORKLOG.md`.
- Keep documentation synchronized with behavioral or contract changes.
- State clearly when a task is documentation-only and no code was modified.
