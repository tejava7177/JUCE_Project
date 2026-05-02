# AGENTS

## JUCE Plugin Agent

- Target: `/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin`
- Scope: JUCE C++ plugin implementation
- Responsibilities:
  - `AudioProcessor` logic
  - `AudioProcessorEditor` UI
  - parameter definitions and synchronization
  - plugin-side state handling
  - client-side server API request integration
  - mapping returned recommendations into plugin controls or UI flows
- Constraints:
  - do not perform network calls inside `processBlock()`
  - do not perform heavy or blocking work on the audio thread
  - keep real-time audio processing isolated from asynchronous analysis tasks
  - use background or message-thread safe coordination for API-triggered workflows

## LLM Server Agent

- Target: `/Users/simjuheun/Desktop/Team/mixing-extension`
- Scope: analysis server implementation
- Responsibilities:
  - LLM-based mixing analysis API design and implementation
  - JSON request parsing and response serialization
  - model API integration
  - recommendation generation for EQ, gain, compressor, and related controls
  - server-side validation and error handling
- Constraints:
  - preserve response schema stability once consumed by the plugin
  - return explicit, machine-readable fields in addition to human-readable summaries

## API Contract Reviewer Agent

- Target: cross-project interface between plugin and server
- Scope: contract consistency review
- Responsibilities:
  - verify request field names and value shapes
  - verify response schema consistency
  - confirm that plugin expectations match server outputs
  - detect breaking changes between implementation and documentation
  - require updates to `API_CONTRACT.md` when schema changes are introduced

## Build & Test Agent

- Target: project-level build and verification workflow
- Scope: build, test, and validation documentation
- Responsibilities:
  - document how to build the JUCE plugin project
  - document how to run server-side tests where applicable
  - suggest validation commands for local verification
  - check that plugin changes, API changes, and integration assumptions are testable
- Output Expectations:
  - provide concrete build commands when implementation work is performed
  - provide test or verification suggestions even when no automated tests exist
