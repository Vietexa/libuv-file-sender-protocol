#include "include/alloc.h"

#include "include/data_structures.h"
#include "stdlib.h"

// Purpose: Helper function to reallocate the clients
int realloc_clients(app_ctx_t *ctx){
size_t old_capacity = ctx->clients.client_capacity;
size_t new_capacity = old_capacity * 2;

peer_t **tmp = realloc(ctx->clients.peers, sizeof(*ctx->clients.peers) * new_capacity);

if (tmp){
    ctx->clients.peers = tmp;
    ctx->clients.client_capacity = new_capacity;

    for (size_t i = old_capacity; i < new_capacity; i++){
        ctx->clients.peers[i] = calloc(1, sizeof(*ctx->clients.peers[i]));
    }
    return 0;
}
else{
    fprintf(stderr, "realloc_clients: There was a problem reallocating space\n");
    return 1;
}
}