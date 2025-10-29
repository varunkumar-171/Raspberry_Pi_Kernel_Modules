#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/errno.h>

#define BUFFER_SIZE 32

/*
 *  malloc buffer
*/
int *buffer;

/* linked list data*/
typedef struct cluster_node{
	int id;
	const char* name;
	struct list_head node;
} cluster_node_t;

/*linked list*/
typedef struct cluster{
	struct list_head nodes;
} cluster_t;

static int room_id = 0;
cluster_t floor_cluster;

/*list init function*/
static int list_init(cluster_t * my_cluster){
	INIT_LIST_HEAD(&my_cluster->nodes);
	return 0;
}

/*Adding to the linked list*/
static int add_to_list(cluster_node_t * cluster_node, cluster_t * my_cluster){
	list_add(&cluster_node->node, &my_cluster->nodes);
	pr_info("Added %s with id %d to the list\n", cluster_node->name, cluster_node->id);
	return 0;
}

/*Deleting from the list*/
static int remove_from_list(cluster_node_t * cluster_node){
	list_del(&cluster_node->node);
	pr_info("Removed %s with id %d to the list\n", cluster_node->name, cluster_node->id);
	return 0;
}

static int free_list(cluster_t * my_cluster){
        cluster_node_t * dummy_node, * curr_node;      /* temporary storage for safe iteration */

        list_for_each_entry_safe(curr_node, dummy_node, &my_cluster->nodes, node) {
		pr_info("Removing %s with id %d from list\n", curr_node->name, curr_node->id);
        	list_del(&curr_node->node);
		kfree(curr_node);
        }
	return 0;
}


static int __init my_init(void)
{

	cluster_node_t * room1, * room2;

	buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL | __GFP_ZERO); // Zero out the memory. Can also use kzmalloc().
	if(!buffer){
		pr_warn("Could not dynmically allocate buffer (ENOMEM) !!\n");
		return ENOMEM;
	}
	pr_info("Succesfully allocated %d bytes of memory\n", BUFFER_SIZE);


	list_init(&floor_cluster);

	/* Adding to list - Test*/
	room1 = kzalloc(sizeof(*room1), GFP_KERNEL);
	room1->name = "Person1";
	room1->id = ++room_id;
	if(!room1){
		pr_warn("Could not create room in cluster (ENOMEM) !!\n");
		return ENOMEM;
	}
	add_to_list(room1, &floor_cluster);

	room2 = kzalloc(sizeof(*room2), GFP_KERNEL);
	room2->name  = "Person2";
	room2->id = ++room_id;
	if(!room2){
		pr_warn("Could not create room in cluster (ENOMEM) !!\n");
		return ENOMEM;
	}
	add_to_list(room2, &floor_cluster);

	/*Removing from list - Test*/
	// remove_from_list(room1);


	return 0;
}

static void __exit my_exit(void)
{
	kfree(buffer);
	pr_info("Released allocated buffer\n");

	free_list(&floor_cluster);
}

module_init(my_init);
module_exit(my_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Varun Kumar");
MODULE_DESCRIPTION("A simple demonstration of kernel module api's");
