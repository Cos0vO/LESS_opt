#include <stdio.h>
#include <math.h>
#include <string.h>
#include <inttypes.h>

#include "cycles.h"
#include "sort.h"
#include "test_helpers.h"

#define ITERS (1u << 14u)

#ifdef N_pad
#define NN N_pad
#else
#define NN N
#endif


void generator_copy(generator_mat_t *V1,
                    const generator_mat_t *V2) {
    memcpy(V1->values, V2->values, sizeof(generator_mat_t));
}

void generator_rnd_fullrank(generator_mat_t *G,
                            uint8_t *is_pivot_column) {
     do {
         generator_rnd(G);
         memset(is_pivot_column, 0, NN);
     } while ( generator_RREF(G,is_pivot_column) == 0);
}

int bench_rref(void) {
    generator_mat_t G2, G1, G3, G4, Gprobe;
    uint8_t is_pivot_column[NN] = {0};
    uint8_t probe_pivot_column[NN] = {0};
    uint8_t g_initial_pivot_flags [NN] = {0};
    uint8_t g_permuted_pivot_flags [NN];

    monomial_t q;

    init_randombytes((const unsigned char *) "rref_123", 8);
    generator_rnd_fullrank(&G2, is_pivot_column);

    setup_cycle_counter();
	printf("rref:\n");
    uint64_t c1 = 0, c2 = 0, c3 = 0, c4 = 0;
    uint64_t ctr = 0, wrapper_ctr = 0, infoset_ctr = 0, start_cycle;
    uint64_t shortcut_successes = 0;

    for (unsigned i = 0; i < ITERS; i++) {
        monomial_mat_rnd(&q);
        generator_monomial_mul(&G1, &G2, &q);
        generator_copy(&G3, &G1);
        generator_copy(&G4, &G1);
        generator_copy(&Gprobe, &G1);
        memset(is_pivot_column, 0, NN);
        memset(probe_pivot_column, 0, NN);

        shortcut_successes += generator_RREF_prefix_infoset(&Gprobe, probe_pivot_column);

    	start_cycle = read_cycle_counter();
        ctr += generator_RREF(&G1, is_pivot_column);
        c1 += (read_cycle_counter() - start_cycle);

        memset(is_pivot_column, 0, NN);
        start_cycle = read_cycle_counter();
        wrapper_ctr += generator_RREF_prefix_infoset_or_fallback(&G3, is_pivot_column);
        c3 += (read_cycle_counter() - start_cycle);

        memset(is_pivot_column, 0, NN);
        start_cycle = read_cycle_counter();
        infoset_ctr += generator_RREF_infoset_systematic_or_fallback(&G4, is_pivot_column);
        c4 += (read_cycle_counter() - start_cycle);
    }
    printf("normal: %0.2f cycles, ctr: %" PRIu64 "\n", (double) c1 / (double) ITERS, ctr);
    printf("prefix infoset wrapper: %0.2f cycles, ctr: %" PRIu64 "\n",
           (double) c3 / (double) ITERS,
           wrapper_ctr);
    printf("prefix infoset successes: %" PRIu64 "/%u, fallbacks: %" PRIu64 "/%u\n",
           shortcut_successes,
           ITERS,
           (uint64_t)ITERS - shortcut_successes,
           ITERS);
    printf("prefix infoset factor %0.3f\n", (double) c3 / (double) c1);
    printf("infoset systematic wrapper: %0.2f cycles, ctr: %" PRIu64 "\n",
           (double) c4 / (double) ITERS,
           infoset_ctr);
    printf("infoset systematic factor %0.3f\n", (double) c4 / (double) c1);

    init_randombytes((const unsigned char *) "rref_123", 8);
    generator_rnd_fullrank(&G2, g_initial_pivot_flags);
    ctr = 0;
    for (unsigned i = 0; i < ITERS; i++) {
        memset(is_pivot_column, 0, NN);
        monomial_mat_rnd(&q);
        generator_monomial_mul(&G1, &G2, &q);
        for (unsigned j = 0; j < N; j++) {
            g_permuted_pivot_flags[q.permutation[j]] = g_initial_pivot_flags[j];
        }

        start_cycle = read_cycle_counter();
        ctr += generator_RREF_pivot_reuse(&G1, is_pivot_column, g_permuted_pivot_flags, VERIFY_PIVOT_REUSE_LIMIT);
        c2 += (read_cycle_counter() - start_cycle);
    }

    printf("reuse: %0.2f cycles, ctr: %" PRIu64 "\n", (double) c2 / (double) ITERS, ctr);
    printf("factor %0.3f\n\n", (double) c2 / (double) c1);
    return 0;
}


int main(void) {
    if (bench_rref()) return 1;
    return 0;
}
