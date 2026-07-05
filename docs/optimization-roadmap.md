# Optimization Roadmap

## RREF information-set shortcut

Detailed notes live in `docs/experiments/rref-information-set.md`. This
experiment keeps the protocol default unchanged and adds a side path that first
tries the special case where columns `0..K-1` form an information set, then
falls back to the existing RREF implementation if the prefix block is singular.
