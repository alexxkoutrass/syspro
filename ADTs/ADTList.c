///////////////////////////////////////////////////////////
//
// Υλοποίηση του ADT List μέσω συνδεδεμένης λίστας.
//
///////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ADTList.h"

struct list {
	ListNode dummy;				// χρησιμοποιούμε dummy κόμβο, ώστε ακόμα και η κενή λίστα να έχει έναν κόμβο.
	ListNode last;				// δείκτης στον τελευταίο κόμβο, ή στον dummy (αν η λίστα είναι κενή)
	int size;					// μέγεθος, ώστε η list_size να είναι Ο(1)
	DestroyFunc destroy_value;	// Συνάρτηση που καταστρέφει ένα στοιχείο της λίστας.
};

struct list_node {
	ListNode next;		// Δείκτης στον επόμενο
	Pointer value;		// Η τιμή που αποθηκεύουμε στον κόμβο
};


List list_create(DestroyFunc destroy_value) {
	// Πρώτα δημιουργούμε το stuct
	List list = malloc(sizeof(*list));
	list->size = 0;
	list->destroy_value = destroy_value;

	// Χρησιμοποιούμε dummy κόμβο, ώστε ακόμα και μια άδεια λίστα να έχει ένα κόμβο
	// (απλοποιεί τους αλγορίθμους). Οπότε πρέπει να τον δημιουργήσουμε.
	//
	list->dummy = malloc(sizeof(*list->dummy));
	list->dummy->next = NULL;		// άδεια λίστα, ο dummy δεν έχει επόμενο

	// Σε μια κενή λίστα, τελευταίος κόμβος είναι επίσης ο dummy
	list->last = list->dummy;

	return list;
}

int list_size(List list) {
	return list->size;
}

void list_insert(List list, Pointer value) {
    ListNode new = malloc(sizeof(*new));
    
    new->value = value;

    list->last->next = new;
    list->last = new;
    list->size++;
}

ListNode list_previous(List list, ListNode current_node) {
    // Check if the list or current_node is NULL or if current_node is the dummy node
    if (current_node == NULL || list == NULL || list->dummy == NULL || current_node == list->dummy) {
        return NULL;
    }

    // Start from the dummy node and traverse the list to find the previous node
    for (ListNode node = list->dummy; node->next != NULL; node = node->next) {
        if (node->next == current_node) {
            return node;  // Found the previous node
        }
    }

    // If no previous node is found, return NULL
    return NULL;
}


void list_delete(List list, ListNode node) {
    // Ensure we're not trying to delete the dummy node or a NULL node
    if (node == NULL) {
        printf("Cannot delete a NULL node.\n");
        return;
    }
    if (node == list->dummy) {
        printf("Cannot delete the dummy node.\n");
        return;
    }

    ListNode prev = list_previous(list, node);

    // If prev is NULL, node is the first real element (right after dummy)
    if (prev == NULL) {
        list->dummy->next = node->next;  // Adjust dummy->next for the first element
    } else {
        prev->next = node->next;  // Adjust the previous node's next pointer
    }

    // Check if the node to delete is the last node
    if (node == list->last) {
        if (prev == NULL) {
            // If deleting the only node, set last and dummy->next to reflect an empty list
            list->last = list->dummy;
            list->dummy->next = NULL;
        } else {
            // Set the last pointer to the previous node
            list->last = prev;
        }
    }

    // If there's a destroy function, call it for the node's value
    if (list->destroy_value != NULL) {
        list->destroy_value(node->value);
    }

    free(node);  // Free the node itself
    list->size--;  // Decrease the size
}





DestroyFunc list_set_destroy_value(List list, DestroyFunc destroy_value) {
	DestroyFunc old = list->destroy_value;
	list->destroy_value = destroy_value;
	return old;
}

void list_destroy(List list) {
	// Διασχίζουμε όλη τη λίστα και κάνουμε free όλους τους κόμβους,
	// συμπεριλαμβανομένου και του dummy!
	//
	ListNode node = list->dummy;
	while (node != NULL) {				// while αντί για for, γιατί θέλουμε να διαβάσουμε
		ListNode next = node->next;		// το node->next _πριν_ κάνουμε free!

		// Καλούμε τη destroy_value, αν υπάρχει (προσοχή, όχι στον dummy!)
		if (node != list->dummy && list->destroy_value != NULL)
			list->destroy_value(node->value);

		free(node);
		node = next;
	}

	// Τέλος free το ίδιο το struct
	free(list);
}


// Διάσχιση της λίστας /////////////////////////////////////////////

ListNode list_first(List list) {
	// Ο πρώτος κόμβος είναι ο επόμενος του dummy.
	//
	return list->dummy->next;
}

ListNode list_last(List list) {
	// Προσοχή, αν η λίστα είναι κενή το last δείχνει στον dummy, εμείς όμως θέλουμε να επιστρέψουμε NULL, όχι τον dummy!
	//
	if (list->last == list->dummy)
		return NULL;		// κενή λίστα
	else
		return list->last;
}

ListNode list_next(List list, ListNode node) {
	assert(node != NULL);	// LCOV_EXCL_LINE (αγνοούμε το branch από τα coverage reports, είναι δύσκολο να τεστάρουμε το false γιατί θα κρασάρει το test)
	return node->next;
}

Pointer list_node_value(List list, ListNode node) {
	assert(node != NULL);	// LCOV_EXCL_LINE
	return node->value;
}

ListNode list_find_node(List list, Pointer value, CompareFunc compare) {
	// διάσχιση όλης της λίστας, καλούμε την compare μέχρι να επιστρέψει 0
	//
	for (ListNode node = list->dummy->next; node != NULL; node = node->next) {
		if (compare(value, node->value) == 0) {
			return node;		// βρέθηκε
		}
	}
	return NULL;	// δεν υπάρχει
}

Pointer list_find(List list, Pointer value, CompareFunc compare) {
	ListNode node = list_find_node(list, value, compare);
	return node == NULL ? NULL : node->value;
}