/* Copyright (C) 2022 Komeil Majidi.
 */


#ifndef _PS_LISTS_H
#define _PS_LISTS_H
#define MACRO_BEGIN do {
#define MACRO_END   } while (0)
#define list_forall(elt, list)   for (elt=list; elt!=NULL; elt=elt->next)
#define list_find(elt, list, c) \
  MACRO_BEGIN list_forall(elt, list) if (c) break; MACRO_END
#define list_forall2(elt, list, hook) \
  for (elt=list, hook=&list; elt!=NULL; hook=&elt->next, elt=elt->next)

#define list_find2(elt, list, c, hook) \
  MACRO_BEGIN list_forall2(elt, list, hook) if (c) break; MACRO_END
#define _list_forall_hook(list, hook) \
  for (hook=&list; *hook!=NULL; hook=&(*hook)->next)
#define _list_find_hook(list, c, hook) \
  MACRO_BEGIN _list_forall_hook(list, hook) if (c) break; MACRO_END
#define list_insert_athook(elt, hook) \
  MACRO_BEGIN elt->next = *hook; *hook = elt; MACRO_END
#define list_unlink_athook(list, elt, hook) \
  MACRO_BEGIN \
  elt = hook ? *hook : NULL; if (elt) { *hook = elt->next; elt->next = NULL; }\
  MACRO_END
#define list_unlink(listtype, list, elt)      \
  MACRO_BEGIN  	       	       	       	      \
  listtype **_hook;			      \
  _list_find_hook(list, *_hook==elt, _hook);  \
  list_unlink_athook(list, elt, _hook);	      \
  MACRO_END
#define list_prepend(list, elt) \
  MACRO_BEGIN elt->next = list; list = elt; MACRO_END
#define list_append(listtype, list, elt)     \
  MACRO_BEGIN                                \
  listtype **_hook;                          \
  _list_forall_hook(list, _hook) {}          \
  list_insert_athook(elt, _hook);            \
  MACRO_END
#define list_concat(listtype, list, list2)   \
  MACRO_BEGIN                                \
  listtype **_hook;                          \
  _list_forall_hook(list, _hook) {}          \
  *_hook = list2;                            \
  MACRO_END

#define list_unlink_cond(listtype, list, elt, c)     \
  MACRO_BEGIN                                        \
  listtype **_hook;			  	     \
  list_find2(elt, list, c, _hook);                   \
  list_unlink_athook(list, elt, _hook);              \
  MACRO_END

#define list_nth(elt, list, n)                                \
  MACRO_BEGIN                                                 \
  int _x;                         \
  for (_x=(n), elt=list; _x && elt; _x--, elt=elt->next) {}   \
  MACRO_END
  
#define list_nth_hook(elt, list, n, hook)                     \
  MACRO_BEGIN                                                 \
  int _x;                          \
  for (_x=(n), elt=list, hook=&list; _x && elt; _x--, hook=&elt->next, elt=elt->next) {}   \
  MACRO_END

#define list_length(listtype, list, n)                   \
  MACRO_BEGIN          	       	       	       	       	 \
  listtype *_elt;   			 		 \
  n=0;					 		 \
  list_forall(_elt, list) 		 		 \
    n++;				 		 \
  MACRO_END
#define list_index(list, n, elt, c)                      \
  MACRO_BEGIN				 		 \
  n=0;					 		 \
  list_forall(elt, list) {		 		 \
    if (c) break;			 		 \
    n++;				 		 \
  }					 		 \
  if (!elt)				 		 \
    n=-1;				 		 \
  MACRO_END

#define list_count(list, n, elt, c)                      \
  MACRO_BEGIN				 		 \
  n=0;					 		 \
  list_forall(elt, list) {		 		 \
    if (c) n++;				 		 \
  }                                                      \
  MACRO_END

#define list_forall_unlink(elt, list) \
  for (elt=list; elt ? (list=elt->next, elt->next=NULL), 1 : 0; elt=list)

#define list_reverse(listtype, list)            \
  MACRO_BEGIN				 	\
  listtype *_list1=NULL, *elt;			\
  list_forall_unlink(elt, list) 		\
    list_prepend(_list1, elt);			\
  list = _list1;				\
  MACRO_END


#define list_insert_ordered(listtype, list, elt, tmp, cond) \
  MACRO_BEGIN                                               \
  listtype **_hook;                                         \
  _list_find_hook(list, (tmp=*_hook, (cond)), _hook);       \
  list_insert_athook(elt, _hook);                           \
  MACRO_END


#define list_sort(listtype, list, a, b, cond)            \
  MACRO_BEGIN                                            \
  listtype *_newlist=NULL;                               \
  list_forall_unlink(a, list)                            \
    list_insert_ordered(listtype, _newlist, a, b, cond); \
  list = _newlist;                                       \
  MACRO_END


#define list_mergesort(listtype, list, a, b, cond)              \
  MACRO_BEGIN						        \
  listtype *_elt, **_hook1;				    	\
							    	\
  for (_elt=list; _elt; _elt=_elt->next1) {			\
    _elt->next1 = _elt->next;				    	\
    _elt->next = NULL;					    	\
  }							    	\
  do {			                               	    	\
    _hook1 = &(list);				    	    	\
    while ((a = *_hook1) != NULL && (b = a->next1) != NULL ) {  \
      _elt = b->next1;					    	\
      _list_merge_cond(listtype, a, b, cond, *_hook1);      	\
      _hook1 = &((*_hook1)->next1);			    	\
      *_hook1 = _elt;				            	\
    }							    	\
  } while (_hook1 != &(list));                                 	\
  MACRO_END

#define _list_merge_cond(listtype, a, b, cond, result)   \
  MACRO_BEGIN                                            \
  listtype **_hook;					 \
  _hook = &(result);					 \
  while (1) {                                            \
     if (a==NULL) {					 \
       *_hook = b;					 \
       break;						 \
     } else if (b==NULL) {				 \
       *_hook = a;					 \
       break;						 \
     } else if (cond) {					 \
       *_hook = a;					 \
       _hook = &(a->next);				 \
       a = a->next;					 \
     } else {						 \
       *_hook = b;					 \
       _hook = &(b->next);				 \
       b = b->next;					 \
     }							 \
  }							 \
  MACRO_END

#define list_merge_sorted(listtype, list1, list2, a, b, cond)	\
  MACRO_BEGIN							\
  a = list1;							\
  b = list2;							\
  _list_merge_cond(listtype, a, b, cond, list1);		\
  list2 = NULL;							\
  MACRO_END


#define list_uniq_sorted(listtype, list, a, b, cond, less, free_elt)	\
  MACRO_BEGIN								\
  if (list != NULL) {							\
    listtype *_l1 = NULL;						\
    listtype **_hook = &_l1;						\
    b = list;								\
    list_unlink(listtype, list, b);					\
    list_forall_unlink(a, list) {					\
      if (cond) {							\
	if (less) {							\
	  free_elt(b);							\
	  b = a;							\
	} else {							\
	  free_elt(a);							\
	}								\
      } else {								\
	list_insert_athook(b, _hook);					\
        _hook=&(*_hook)->next;						\
	b = a;								\
      }									\
    }									\
    list_insert_athook(b, _hook);					\
    list = _l1;								\
  }									\
  MACRO_END



#define dlist_append(head, end, elt)                    \
  MACRO_BEGIN  	       	       	       	       	       	 \
  elt->prev = end;					 \
  elt->next = NULL;					 \
  if (end) {						 \
    end->next = elt;					 \
  } else {  						 \
    head = elt;						 \
  }	    						 \
  end = elt;						 \
  MACRO_END

#define dlist_forall_unlink(elt, head, end) \
  for (elt=head; elt ? (head=elt->next, elt->next=NULL, elt->prev=NULL), 1 : (end=NULL, 0); elt=head)

#define dlist_unlink_first(head, end, elt)               \
  MACRO_BEGIN				       	       	 \
  elt = head;						 \
  if (head) {						 \
    head = head->next;					 \
    if (head) {						 \
      head->prev = NULL;				 \
    } else {						 \
      end = NULL;					 \
    }    						 \
    elt->prev = NULL;					 \
    elt->next = NULL;					 \
  }							 \
  MACRO_END

#endif 

