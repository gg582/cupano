/**
 * @file swarm_prompt_builder.hpp
 * @brief Builds the six-language cross-referencing swarm prompt.
 */

#pragma once

#include "core/public_dto.hpp"
#include "prompt/regional_agent_registry.hpp"
#include "text/text_normalizer.hpp"
#include "text/text_types.hpp"
#include "wheel/flavor_wheel.hpp"

namespace cupano {

/** Builds the coordinated six-language cross-referencing swarm prompt. */
class SwarmPromptBuilder {
public:
    SwarmPromptBuilder(const RegionalAgentRegistry& registry, const FlavorWheel& wheel, const TextNormalizer& normalizer);

    SwarmPrompt build(const SwarmPromptConfig& config) const;

private:
    TextFragment buildSystemInstruction(const SwarmPromptConfig& config) const;
    TextFragment buildUserPrompt(const SwarmPromptConfig& config) const;
    TextFragment describeBean(const BeanMetadata& bean) const;
    TextFragment describeUser(const UserContext& user) const;
    TextFragment describeAgents(const SwarmPromptConfig& config) const;
    TextFragment buildSearchQuery(const BeanMetadata& bean, const RegionalAgentSpec& agent) const;
    TextFragment describeOutputSchema() const;
    TextFragment quoteUntrusted(const TextFragment& value) const;

    const RegionalAgentRegistry& registry_;
    const FlavorWheel& wheel_;
    const TextNormalizer& normalizer_;
};

}  // namespace cupano
