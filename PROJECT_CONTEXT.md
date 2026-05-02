# PROJECT_CONTEXT

## Overview

This repository root, `/Users/simjuheun/Developer/JUCE_prac`, is used as the shared working context for JUCE-based audio plugin development and related integration planning.

The primary plugin project in this workspace is:

- `/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin`

The external server project that will provide analysis logic is:

- `/Users/simjuheun/Desktop/Team/mixing-extension`

These projects are intentionally kept in separate directories. They must remain separate, and integration should be coordinated through documented API contracts rather than by changing directory layout or merging project structures.

## VoltaAgentPlugin Role

`VoltaAgentPlugin` is the JUCE/C++ audio plugin project. Its responsibilities include:

- audio plugin processing and parameter management
- plugin editor UI and user interaction
- collecting or preparing analysis-ready metadata and feature values
- acting as the client for server API requests outside the real-time audio path
- applying returned recommendation values to plugin-visible controls or workflows

The plugin should be treated as the real-time client application. Audio-thread safety takes priority over convenience.

## mixing-extension Server Role

`mixing-extension` is a separate server-side project responsible for LLM-assisted mixing analysis. Its responsibilities include:

- receiving analysis requests from the plugin
- validating and parsing JSON request payloads
- calling model or inference backends as needed
- generating structured mixing suggestions
- returning stable JSON responses that conform to the shared API contract

The server is the analysis engine and recommendation provider, not the audio processor.

## Planned Integration Model

The intended architecture is:

1. The plugin gathers track context, extracted features, and current parameter state.
2. The plugin sends a request to the server using a documented HTTP API.
3. The server analyzes the request and returns structured suggestions such as EQ, gain, or compressor recommendations.
4. The plugin presents or applies those suggestions in a safe, user-visible workflow.

## Integration Principle

At the current stage, the plugin project and the server project are separated physically and logically:

- plugin code remains in `/Users/simjuheun/Developer/JUCE_prac/VoltaAgentPlugin`
- server code remains in `/Users/simjuheun/Desktop/Team/mixing-extension`

The source of truth for cross-project connectivity is the API contract document:

- `/Users/simjuheun/Developer/JUCE_prac/API_CONTRACT.md`

Any future plugin-server connection work should follow the API contract first, then implementation details in each project. Directory paths must not be changed arbitrarily.
