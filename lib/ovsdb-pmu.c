/* Copyright (C) 2016 Hewlett Packard Enterprise Development LP
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

#include <config.h>
#include "ovsdb-pmu.h"
#include "util.h"
#include "hmap.h"
#include "hash.h"

/* PMU: Partial Map Update */
struct pmu {
    struct hmap_node node;
    struct ovsdb_datum *datum;
    enum pmu_operation operation;
};

/* PMUL: Partial Map Update List */
struct pmul {
    struct hmap hmap;
};

struct pmu* pmul_find_pmu(struct pmul *, struct pmu *,
                          const struct ovsdb_type *, size_t);
void pmu_destroy_datum(struct pmu *, const struct ovsdb_type *);

struct pmu*
pmu_create(struct ovsdb_datum *datum, enum pmu_operation operation)
{
    struct pmu *pmu = xmalloc(sizeof *pmu);
    pmu->node.hash = 0;
    pmu->node.next = HMAP_NODE_NULL;
    pmu->datum = datum;
    pmu->operation = operation;
    return pmu;
}

void
pmu_destroy_datum(struct pmu *pmu, const struct ovsdb_type *type)
{
    if (pmu->operation == PMU_DELETE){
        struct ovsdb_type type_ = *type;
        type_.value.type = OVSDB_TYPE_VOID;
        ovsdb_datum_destroy(pmu->datum, &type_);
    } else {
        ovsdb_datum_destroy(pmu->datum, type);
    }
    pmu->datum = NULL;
}

void
pmu_destroy(struct pmu *pmu, const struct ovsdb_type *type)
{
    pmu_destroy_datum(pmu, type);
    free(pmu);
}

struct ovsdb_datum*
pmu_datum(const struct pmu *pmu)
{
    return pmu->datum;
}

enum pmu_operation
pmu_operation(const struct pmu *pmu)
{
    return pmu->operation;
}

struct pmul*
pmul_create()
{
    struct pmul *list = xmalloc(sizeof *list);
    hmap_init(&list->hmap);
    return list;
}

void
pmul_destroy(struct pmul *list, const struct ovsdb_type *type)
{
    struct pmu *pmu, *next;
    HMAP_FOR_EACH_SAFE (pmu, next, node, &list->hmap) {
        pmu_destroy(pmu, type);
    }
    hmap_destroy(&list->hmap);
    free(list);
}

struct pmu*
pmul_find_pmu(struct pmul *list, struct pmu *pmu,
              const struct ovsdb_type *type, size_t hash)
{
    struct pmu *found = NULL;
    struct pmu *old;
    HMAP_FOR_EACH_WITH_HASH(old, node, hash, &list->hmap) {
        if (ovsdb_atom_equals(&old->datum->keys[0],
                              &pmu->datum->keys[0],type->key.type)) {
            found = old;
            break;
        }
    }
    return found;
}

/* Inserts 'pmu' into 'list'. Makes sure that any conflict with a previous
 * update is resolved, so only one update operation is possible on each
 * key per transactions. 'type' must be the type of the column over which the
 * partial map updates will be applied. */
void
pmul_add_pmu(struct pmul *list, struct pmu *pmu, const struct ovsdb_type *type)
{
    /* Check if there is a previous update with the same key. */
    size_t hash = ovsdb_atom_hash(&pmu->datum->keys[0], type->key.type, 0);
    struct pmu *prev_pmu = pmul_find_pmu(list, pmu, type, hash);
    if (prev_pmu == NULL){
        hmap_insert(&list->hmap, &pmu->node, hash);
    } else {
        if (prev_pmu->operation == PMU_INSERT &&
            pmu->operation == PMU_DELETE) {
            /* These update operations cancel each other out. */
            hmap_remove(&list->hmap, &prev_pmu->node);
            pmu_destroy(prev_pmu, type);
            pmu_destroy(pmu, type);
        } else {
            /* For any other case, the new update operation replaces
             * the previous update operation. */
            pmu_destroy_datum(prev_pmu, type);
            prev_pmu->operation = pmu->operation;
            prev_pmu->datum = pmu->datum;
            free(pmu);
        }
    }
}

struct pmu*
pmul_first(struct pmul *list)
{
    struct hmap_node *node = hmap_first(&list->hmap);
    if (node == NULL) {
        return NULL;
    }
    struct pmu *pmu = CONTAINER_OF(node, struct pmu, node);
    return pmu;
}

struct pmu* pmul_next(struct pmul *list, struct pmu *pmu){
    struct hmap_node *node = hmap_next(&list->hmap, &pmu->node);
    if (node == NULL) {
        return NULL;
    }
    struct pmu *next = CONTAINER_OF(node, struct pmu, node);
    return next;
}
