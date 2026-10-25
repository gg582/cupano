/**
 * @file normalizer.hpp
 * @brief Umbrella header of the cupano Wasm core: the public DTOs and the
 *        NormalizerFacade, i.e. everything JavaScript can see.
 *
 * Source layout (one class per header/source pair):
 *   core/      public DTOs
 *   text/      UTF-8 helpers and the generic alias lexicon
 *   json/      JSON model, extractor, parser, reader
 *   roast/     fixed roast table, scale and cross-regional consensus
 *   page/      product-page sanitizer and its lexicons
 *   prompt/    regional agent registry and swarm prompt builder
 *   analysis/  response parsing, frequency/availability analysis, normalizer
 *   facade/    composition root exported via bindings.cpp
 *
 * Design rules enforced in this module:
 *  - Every public input/output is an explicit DTO (no loose primitives).
 *  - Each class has a single responsibility; collaborators are injected by
 *    reference from the composition root (NormalizerFacade).
 *  - Dependencies flow strictly downward: the facade calls services, services
 *    call lexicons/parsers, and nothing ever calls back up.
 *  - No global or static mutable state. All lookup tables are built inside
 *    instance constructors and owned by the facade instance.
 *  - Flavor notes are never parsed with fixed vocabularies. The LLM reads the
 *    sources and supplies each note's canonical term; this core only performs
 *    frequency analysis over what the LLM returned.
 */
#pragma once

#include "core/public_dto.hpp"
#include "facade/normalizer_facade.hpp"
