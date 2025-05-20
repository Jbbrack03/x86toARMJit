#include "xenoarm_jit/translation_cache/translation_cache.h"
#include "logging/logger.h"
#include <iostream> // For std::cerr and std::endl
#include <cstring>
#include <algorithm>

namespace xenoarm_jit {
namespace translation_cache {

TranslationCache::TranslationCache() {
    LOG_DEBUG("TranslationCache created");
}

TranslationCache::~TranslationCache() {
    // Free all blocks in the cache
    flush();
    LOG_DEBUG("TranslationCache destroyed");
}

TranslatedBlock* TranslationCache::lookup(uint64_t guest_address) {
    LOG_DEBUG("Looking up guest address 0x" + std::to_string(guest_address) + " in TranslationCache.");
    auto it = cache_.find(guest_address);
    if (it != cache_.end()) {
        LOG_DEBUG("Found translated block for guest address 0x" + std::to_string(guest_address) + ".");
        return it->second;
    }
    LOG_DEBUG("No translated block found for guest address 0x" + std::to_string(guest_address) + ".");
    return nullptr;
}

void TranslationCache::store(TranslatedBlock* block) {
    if (!block) {
        LOG_ERROR("Attempted to store a null TranslatedBlock.");
        return;
    }
    LOG_DEBUG("Storing translated block for guest address 0x" + std::to_string(block->guest_address) + ".");

    // Check for existing block at this address
    auto it = cache_.find(block->guest_address);
    if (it != cache_.end()) {
        LOG_WARNING("Overwriting existing translated block for guest address 0x" + std::to_string(block->guest_address) + ".");
        // Invalidate the old block (breaks chains, etc)
        invalidate(block->guest_address);
    }

    // Store the new block
    cache_[block->guest_address] = block;
}

void TranslationCache::chain_blocks(TranslatedBlock* block, 
    std::function<void(TranslatedBlock*, TranslatedBlock*, const TranslatedBlock::ControlFlowExit&)> patch_callback) {
    
    if (!block) {
        LOG_ERROR("Attempted to chain a null TranslatedBlock.");
        return;
    }
    
    LOG_DEBUG("Chaining block at guest address 0x" + std::to_string(block->guest_address) + ".");
    
    // For each exit in the block
    for (auto& exit : block->exits) {
        // Only chain deterministic exits
        if (exit.type == TranslatedBlock::ControlFlowExitType::JMP || 
            exit.type == TranslatedBlock::ControlFlowExitType::BR_COND || 
            exit.type == TranslatedBlock::ControlFlowExitType::FALLTHROUGH) {
            
            // Try to find the target block
            TranslatedBlock* target_block = lookup(exit.target_guest_address);
            if (target_block && !exit.is_patched) {
                LOG_DEBUG("Chaining block at 0x" + std::to_string(block->guest_address) + 
                          " to block at 0x" + std::to_string(target_block->guest_address) + ".");
                
                // Call the patch callback to perform the actual patching
                patch_callback(block, target_block, exit);
                
                // Mark this exit as patched
                exit.is_patched = true;
                
                // Track incoming links (for invalidation)
                target_block->incoming_links.insert(block);
                block->is_linked = true;
            }
            
            // For conditional branches, also check the false target
            if (exit.type == TranslatedBlock::ControlFlowExitType::BR_COND) {
                TranslatedBlock* target_block_false = lookup(exit.target_guest_address_false);
                if (target_block_false) {
                    LOG_DEBUG("Chaining block at 0x" + std::to_string(block->guest_address) + 
                              " false path to block at 0x" + std::to_string(target_block_false->guest_address) + ".");
                    
                    // TODO: We need a separate callback for patching the false path
                    // For now, it will be handled in the same callback
                    
                    // Track incoming links (for invalidation)
                    target_block_false->incoming_links.insert(block);
                    block->is_linked = true;
                }
            }
        }
    }
}

void TranslationCache::unchain_block(TranslatedBlock* block) {
    if (!block || !block->is_linked) {
        return;
    }
    
    LOG_DEBUG("Unchaining block at guest address 0x" + std::to_string(block->guest_address) + ".");
    
    // Remove this block from the incoming_links of all its targets
    for (const auto& exit : block->exits) {
        if (exit.is_patched) {
            // Find target block
            TranslatedBlock* target = lookup(exit.target_guest_address);
            if (target) {
                // Remove this block from its incoming links
                target->incoming_links.erase(block);
            }
            
            // For conditional branches, also handle the false path
            if (exit.type == TranslatedBlock::ControlFlowExitType::BR_COND) {
                TranslatedBlock* target_false = lookup(exit.target_guest_address_false);
                if (target_false) {
                    // Remove this block from its incoming links
                    target_false->incoming_links.erase(block);
                }
            }
        }
    }
    
    // Break all incoming links to this block
    for (TranslatedBlock* incoming : block->incoming_links) {
        // Reset patched flag for all exits pointing to this block
        for (auto& exit : incoming->exits) {
            if ((exit.target_guest_address == block->guest_address) || 
                (exit.type == TranslatedBlock::ControlFlowExitType::BR_COND && 
                 exit.target_guest_address_false == block->guest_address)) {
                exit.is_patched = false;
            }
        }
    }
    
    // Clear incoming links
    block->incoming_links.clear();
    block->is_linked = false;
}

void TranslationCache::invalidate(uint64_t guest_address) {
    TranslatedBlock* block = lookup(guest_address);
    if (block) {
        LOG_DEBUG("Invalidating block at guest address 0x" + std::to_string(guest_address) + ".");
        
        // Break all chains to and from this block
        unchain_block(block);
        
        // Remove from cache and delete
        cache_.erase(guest_address);
        delete block;
    }
}

void TranslationCache::invalidate_range(uint64_t start_address, uint64_t end_address) {
    LOG_DEBUG("Invalidating blocks in range 0x" + std::to_string(start_address) + 
              " to 0x" + std::to_string(end_address) + ".");
    
    // Collect blocks to invalidate
    std::vector<uint64_t> to_invalidate;
    
    for (const auto& entry : cache_) {
        uint64_t block_start = entry.second->guest_address;
        uint64_t block_end = block_start + entry.second->guest_size;
        
        // Check if block overlaps with the invalidation range
        if ((block_start <= end_address) && (block_end >= start_address)) {
            to_invalidate.push_back(entry.first);
        }
    }
    
    // Now invalidate them all
    for (uint64_t addr : to_invalidate) {
        invalidate(addr);
    }
}

size_t TranslationCache::get_chained_block_count() const {
    size_t count = 0;
    for (const auto& entry : cache_) {
        if (entry.second->is_linked) {
            count++;
        }
    }
    return count;
}

void TranslationCache::flush() {
    LOG_DEBUG("Flushing translation cache.");
    
    // Delete all blocks
    for (auto const& [address, block] : cache_) {
        delete block;
    }
    cache_.clear();
}

} // namespace translation_cache
} // namespace xenoarm_jit