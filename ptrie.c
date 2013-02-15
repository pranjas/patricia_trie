#include "ptrie.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

static struct ptrie_node* new_node(struct ptrie_node *node,int sibling)
{
	struct list_head *list;
	struct ptrie_node *new_node;
	if(!node) {
		new_node = calloc(1,sizeof(struct ptrie_node));
		if(!new_node)
			return NULL;
		INIT_LIST_HEAD(&new_node->next);
		INIT_LIST_HEAD(&new_node->sibling_list);
		return new_node;	
	}
	if(sibling) 
		list = &node->sibling_list;
	else 
		list = &node->next;
	new_node = calloc(1,sizeof(struct ptrie_node));
	if (!new_node) {
		return NULL;
	}
	INIT_LIST_HEAD(&new_node->next);
	INIT_LIST_HEAD(&new_node->sibling_list);
	list_add_tail(&new_node->sibling_list,list);
return new_node;
}

static int __add_word(struct ptrie_node *node,const char *word)
{
	/*
	 * Perhaps we can do away with this condition. I'm not
	 * bothering to change it for the moment.
	 * */
	if (node->substr && strlen(word)<=strlen(node->substr)) {
		strcpy(node->substr,word);
		return;
	}
	if (node->substr) {
		free(node->substr);
		node->substr = NULL;
	}
	node->substr = calloc(1,strlen(word)+1);
	if (!node->substr)
		return ENOMEM;
	strcpy(node->substr,word);
return 0;
}

/*
 * Split the given ptrie_node into two. One just stays the same
 * only that its substr is shrunk. The other part goes into the
 * next list of this ptrie_node.
 * */
static int split_trie(struct ptrie_node *node,ssize_t offset)
{
	struct ptrie_node *nnode = new_node(node,0);
	char *substr;
	if (!nnode)
		return ENOMEM;
	if (__add_word(nnode,node->substr+offset)) {
		free_node(nnode);
		return ENOMEM;
	}
	*(node->substr+offset) = '\0';
	substr = calloc(1,strlen(node->substr)+1);
	if (!substr)
		return ENOMEM;
	strcpy(substr,node->substr);
	free(node->substr);
	node->substr = substr;
return 0;
}

/*
 * I would really like to change this. Recursive functions suck...
 * but perhaps we can do a controlled recursion by maintaining depth of 
 * recursion? It would require stack/queue for book keeping and a stupid
 * static variable but i'm pretty sure it can be done. Well perhaps you can 
 * try it... I got busy with some other stuff :P.
 * */
void free_node(struct ptrie_node *node)
{
	struct list_head *cur,*next;
	list_for_each_safe(cur,next,&node->next) {
		struct ptrie_node *item = list_entry(cur,struct ptrie_node
					,sibling_list);
		free_node(item);
		list_del_init(&item->sibling_list);
		if(item->substr)
			free(item->substr);
		free(item);
	}
	if(node->substr)
		free(node->substr);
	list_del_init(&node->sibling_list);
	free(node);
}

/*
 * Find the longest matching prefix length between two strings.
 * See below how it's used.
 * */
static int __match_index(const char *str1,const char *str2)
{
	int indx = 0;
	while (*str1 && *str2) {
		if(*str1++ == *str2++)
			indx++;
		else	break;
	}
	return indx;
}
/*
 * If status is set then it means it's a perfect match. Otherwise
 * the returned ptrie_node is the best match we've got till now.
 * The best match can also be null in which case the root of the
 * ptrie needs to be split.
 *
 * The logic is pretty much straight forward, beginning root try to
 * find the best matching substring. From there move in that node
 * and do the same until that string is found.
 */
static struct ptrie_node* __search(struct ptrie_root *root,const char *word,
					int *status)
{
	struct list_head *cur,*next;
	struct list_head *tmp_head = &root->next;
	off_t offset = 0;	
	struct ptrie_node *item=NULL;
	struct ptrie_node *most_matched = NULL;
	int last_most_matched_index = 0,cur_match_indx = 0,substr_len;
	*status = 0;
search_again:
	list_for_each_safe(cur,next,tmp_head) {
		item = list_entry(cur,struct ptrie_node,sibling_list);
		if (item->substr) {
			/*
			 * Match only the characters that matter.
			 * */
			substr_len = strlen(word)>strlen(item->substr)?
					strlen(item->substr):strlen(word);

			if(!strncmp(word + offset,item->substr,substr_len)) {
				/*
				 * Marks the fact that beginning this node
				 * the search would be like doing the search
				 * beginning from root.
				 * */
				last_most_matched_index = 0;
				most_matched = item;
				offset += substr_len;
				if(offset == strlen(word)) {
					*status = 1;
					goto done;
				}
				else { /*Move into the list of node*/
					tmp_head = &item->next;
					goto search_again;
				}
			}
			else {
				if (last_most_matched_index <
					(cur_match_indx = 
					__match_index(word+offset,item->substr)
						)) {
						last_most_matched_index =
							cur_match_indx;
						most_matched = item;
				}
			}
		}
	}
	return most_matched; /*Not Successfull.*/
done:
	return item;/*Gotcha...*/
}

int search(struct ptrie_root *root,const char *word)
{
	int found;
	if(!word) return 0;
	__search(root,word,&found);
	return found;
}

int add_word(struct ptrie_root *root,const char *word)
{
	int found;
	struct ptrie_node *split_node = __search(root,word,&found);
	struct ptrie_node *nnode = NULL;
	int offset = 0;
	if (!found) {
		if(split_node) {
			/*
			 * Can you make "" strings not to appear
			 * in ptrie_node?. Just a simple line of code
			 * can make it happen so I'll leave it to you
			 * to figure that out.
			 * */
			offset = __match_index(word,split_node->substr);
			if(split_trie(split_node,offset))
				return ENOMEM;
		}
		nnode = new_node(split_node,0);
		if (!nnode) 
			return ENOMEM;
		if(!split_node)
			list_add_tail(&nnode->sibling_list,&root->next);
		__add_word(nnode,word+offset);
	}
	return 0;
}

/*
 * You probably would want it to be in a proper test file. I'm very lazy
 * when testing so here it goes... :P.
 * */
#define TEST_ELEMENTS 25
const char *words[] ={	"kal","kaleidoscope","pappu","pager","pandu",
			"bizkit","linkin","park","gaming","gamer",
			"sweater","sweat","what","wheat","cambodia",
			"linux","unix","flower","flavour","holy",
			"kizashi","ferrari","enferno","studio","limp"
			};
int main()
{
	struct ptrie_root proot;
	INIT_PTRIE_ROOT(&proot);
	int i=0;
	for(;i<TEST_ELEMENTS;i++) {
		add_word(&proot,words[i]);
	}
	for(i-=1;i>=0;i--) {
		if(search(&proot,words[i]))
			printf("Word %s is present\n",words[i]);
	}
}
