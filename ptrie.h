#ifndef PTRIE_H
#define PTRIE_H
#include "list.h"

struct ptrie_node {
	struct list_head next;
	struct list_head sibling_list;
	char	*substr;
};
/*
 * The caller must take care that root node doesn't go away.
 * */
struct ptrie_root {
	struct list_head next;
};

/*
 * The way this is structured is as shown below,
 * ptrie_root
 * 	|--->next->ptrie_node-->ptrie_node
 * 			|	|
 * 			|	|--->ptrie_node-->ptrie_node
 * 			|
 * 			|--->ptrie_node-->ptrie_node....
 * */

/*
 * Returns 0 on success.
 * */
int add_word		(struct ptrie_root *node,const char *word);

/*
 * Returns 1 on success.
 * */
int search		(struct ptrie_root *node,const char *word);

/*
 * This one isn't tested yet. Perhaps someone can make it non-recursive?
 * */
void free_node		(struct ptrie_node *node);

#define INIT_PTRIE_ROOT(ROOT)\
	INIT_LIST_HEAD(& ((ROOT)->next))
#endif /*PTRIE_H*/
