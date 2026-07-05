#include <stdio.h>
#include <math.h>
#include <string.h>
#include <inttypes.h>

#include "codes.h"
#include "cycles.h"
#include "sort.h"

#define ITERS (1u << 14u)

/* samples a random generator matrix */
void generator_rnd_local(generator_mat_t *res) {
   for(uint32_t i = 0; i < K; i++) {
      rand_range_q_elements(res->values[i], N);
   }
} /* end generator_rnd */

void monomial_mat_rnd_local(monomial_t *res) {
   fq_star_rnd_elements(res->coefficients, N);
   for(uint32_t i = 0; i < N; i++) {
      res->permutation[i] = i;
   }
   /* FY shuffle on the permutation */
   yt_shuffle(res->permutation);
} /* end monomial_mat_rnd */

void generator_copy(generator_mat_t *V1,
                    const generator_mat_t *V2) {
    memcpy(V1->values, V2->values, sizeof(generator_mat_t));
}

void generator_rnd_fullrank(generator_mat_t *G,
                            uint8_t *is_pivot_column) {
     do {
         generator_rnd_local(G);
         memset(is_pivot_column, 0, N);
     } while ( generator_RREF(G,is_pivot_column) == 0);
}

int bench_rref(void) {
    generator_mat_t G2, G1, G3, Gprobe;
    uint8_t is_pivot_column[N] = {0};
    uint8_t probe_pivot_column[N] = {0};
    uint8_t g_initial_pivot_flags [N] = {0};
    uint8_t g_permuted_pivot_flags [N];

    monomial_t q;

    init_randombytes((const unsigned char *) "rref_123", 8);
    generator_rnd_fullrank(&G2, is_pivot_column);

    setup_cycle_counter();
	printf("rref:\n");
    uint64_t c1 = 0, c2 = 0, c3 = 0, ctr = 0, wrapper_ctr = 0, start_cycle;
    uint64_t shortcut_successes = 0;

    for (unsigned i = 0; i < ITERS; i++) {
        monomial_mat_rnd_local(&q);
        generator_monomial_mul(&G1, &G2, &q);
        generator_copy(&G3, &G1);
        generator_copy(&Gprobe, &G1);
        memset(is_pivot_column, 0, N);
        memset(probe_pivot_column, 0, N);

        shortcut_successes += generator_RREF_prefix_infoset(&Gprobe, probe_pivot_column);

    	start_cycle = read_cycle_counter();
        ctr += generator_RREF(&G1, is_pivot_column);
        c1 += (read_cycle_counter() - start_cycle);

        memset(is_pivot_column, 0, N);
        start_cycle = read_cycle_counter();
        wrapper_ctr += generator_RREF_prefix_infoset_or_fallback(&G3, is_pivot_column);
        c3 += (read_cycle_counter() - start_cycle);
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

    init_randombytes((const unsigned char *) "rref_123", 8);
    generator_rnd_fullrank(&G2, g_initial_pivot_flags);
    ctr = 0;
    for (unsigned i = 0; i < ITERS; i++) {
        memset(is_pivot_column, 0, N);
        monomial_mat_rnd_local(&q);
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
