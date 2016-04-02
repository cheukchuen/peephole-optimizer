/******************************
 * Group comp520-2016-14
 * ============================
 * - Alexandre St-Louis Fortier
 * - Stefan Knudsen
 * - Cheuk Chuen Siow
 ******************************/

/*
 * JOOS is Copyright (C) 1997 Laurie Hendren & Michael I. Schwartzbach
 *
 * Reproduction of all or part of this software is permitted for
 * educational or research use on condition that this copyright notice is
 * included in any copy. This software comes with no warranty of any
 * kind. In no event will the authors be liable for any damages resulting from
 * use of this software.
 *
 * email: hendren@cs.mcgill.ca, mis@brics.dk
 */

/* iload x        iload x        iload x
 * ldc 0          ldc 1          ldc 2
 * imul           imul           imul
 * ------>        ------>        ------>
 * ldc 0          iload x        iload x
 *                               dup
 *                               iadd
 */

int simplify_multiplication_right(CODE **c)
{ int x,k;
  if (is_iload(*c,&x) && 
      is_ldc_int(next(*c),&k) && 
      is_imul(next(next(*c)))) {
     if (k==0) return replace(c,3,makeCODEldc_int(0,NULL));
     else if (k==1) return replace(c,3,makeCODEiload(x,NULL));
     else if (k==2) return replace(c,3,makeCODEiload(x,
                                       makeCODEdup(
                                       makeCODEiadd(NULL))));
     return 0;
  }
  return 0;
}

/* dup
 * astore x
 * pop
 * -------->
 * astore x
 */
int simplify_astore(CODE **c)
{ int x;
  if (is_dup(*c) &&
      is_astore(next(*c),&x) &&
      is_pop(next(next(*c)))) {
     return replace(c,3,makeCODEastore(x,NULL));
  }
  return 0;
}

/* iload x
 * ldc k   (0<=k<=127)
 * iadd
 * istore x
 * --------->
 * iinc x k
 */ 
int positive_increment(CODE **c)
{ int x,y,k;
  if (is_iload(*c,&x) &&
      is_ldc_int(next(*c),&k) &&
      is_iadd(next(next(*c))) &&
      is_istore(next(next(next(*c))),&y) &&
      x==y && 0<=k && k<=127) {
     return replace(c,4,makeCODEiinc(x,k,NULL));
  }
  return 0;
}

/* goto L1
 * ...
 * L1:
 * goto L2
 * ...
 * L2:
 * --------->
 * goto L2
 * ...
 * L1:    (reference count reduced by 1)
 * goto L2
 * ...
 * L2:    (reference count increased by 1)  
 */
int simplify_goto_goto(CODE **c)
{ int l1,l2;
  if (is_goto(*c,&l1) && is_goto(next(destination(l1)),&l2) && l1>l2) {
     droplabel(l1);
     copylabel(l2);
     return replace(c,1,makeCODEgoto(l2,NULL));
  }
  return 0;
}



/*******************************************
 * Group comp520-2016-14's peephole patterns
 *******************************************/

/* 
 * ldc 0          ldc 1          ldc 2
 * iload x        iload x        iload x
 * imul           imul           imul
 * ------>        ------>        ------>
 * ldc 0          iload x        iload x
 *                               dup
 *                               iadd
 */
int simplify_multiplication_left(CODE **c)
{ int x,k;
  if (is_ldc_int(*c,&k) && 
      is_iload(next(*c),&x) && 
      is_imul(nextby(*c,2))) {
     if (k==0) return replace(c,3,makeCODEldc_int(0,NULL));
     else if (k==1) return replace(c,3,makeCODEiload(x,NULL));
     else if (k==2) return replace(c,3,makeCODEiload(x,
                                       makeCODEdup(
                                       makeCODEiadd(NULL))));
     return 0;
  }
  return 0;
}

/* dup
 * istore x
 * pop
 * -------->
 * istore x
 */
int simplify_istore(CODE **c)
{ int x;
  if (is_dup(*c) &&
      is_istore(next(*c),&x) &&
      is_pop(nextby(*c,2))) {
     return replace(c,3,makeCODEistore(x,NULL));
  }
  return 0;
}

/* aload x
 * aload x
 * --------->
 * aload x
 * dup
 */
int simplify_aload(CODE **c)
{ int x1,x2;
  if (is_aload(*c,&x1) &&
      is_aload(next(*c),&x2) &&
      x1 == x2) {
    return replace(c,2,makeCODEaload(x1,
                       makeCODEdup(NULL)));
  }
  return 0;
}

/* iload x
 * iload x
 * --------->
 * iload x
 * dup
 */
int simplify_iload(CODE **c)
{ int x1,x2;
  if (is_iload(*c,&x1) &&
      is_iload(next(*c),&x2) &&
      x1 == x2) {
    return replace(c,2,makeCODEiload(x1,
                       makeCODEdup(NULL)));
  }
  return 0;
}

/* dup
 * aload x
 * swap
 * putfield k
 * pop
 * -------->
 * aload x
 * swap
 * putfield k
 */
int simplify_aload_swap_putfield(CODE **c)
{ int x; char* k;
  if (is_dup(*c) &&
      is_aload(next(*c),&x) &&
      is_swap(nextby(*c,2)) &&
      is_putfield(nextby(*c,3),&k) &&
      is_pop(nextby(*c,4))) {
     return replace(c,5,makeCODEaload(x,
                        makeCODEswap(
                        makeCODEputfield(k,NULL))));
  }
  return 0;
}

/* ldc x
 * ldc y
 * iadd / isub / imul / idiv / irem
 * ------>
 * ldc x (+|-|*|/|%) y
 */
int simplify_constant_op(CODE **c)
{ int x,y,k;
  if (is_ldc_int(*c,&x) &&
      is_ineg(next(*c)) &&
      is_ldc_int(nextby(*c,2),&y) &&
      is_ineg(nextby(*c,3))) {
    if (is_iadd(nextby(*c,4))) {
      k = (-x)+(-y);
    } else if (is_isub(nextby(*c,4))) {
      k = (-x)-(-y);
    } else if (is_imul(nextby(*c,4))) {
      k = (-x)*(-y);
    } else if (is_idiv(nextby(*c,4))) {
      k = (-x)/(-y);
    } else if (is_irem(nextby(*c,4))) {
      k = (-x)%(-y);
    }
    if (k >= 0) {
      return replace(c,5,makeCODEldc_int(k,NULL));
    }
  } else if (is_ldc_int(*c,&x) &&
      is_ldc_int(next(*c),&y) &&
      is_ineg(nextby(*c,2))) {
    if (is_iadd(nextby(*c,3))) {
      k = x+(-y);
    } else if (is_isub(nextby(*c,3))) {
      k = x-(-y);
    } else if (is_imul(nextby(*c,3))) {
      k = x*(-y);
    } else if (is_idiv(nextby(*c,3))) {
      k = x/(-y);
    } else if (is_irem(nextby(*c,3))) {
      k = x%(-y);
    }
    if (k >= 0) {
      return replace(c,4,makeCODEldc_int(k,NULL));
    }
  } else if (is_ldc_int(*c,&x) &&
      is_ineg(next(*c)) &&
      is_ldc_int(nextby(*c,2),&y)) {
    if (is_iadd(nextby(*c,3))) {
      k = (-x)+y;
    } else if (is_isub(nextby(*c,3))) {
      k = (-x)-y;
    } else if (is_imul(nextby(*c,3))) {
      k = (-x)*y;
    } else if (is_idiv(nextby(*c,3))) {
      k = (-x)/y;
    } else if (is_irem(nextby(*c,3))) {
      k = (-x)%y;
    }
    if (k >= 0) {
      return replace(c,4,makeCODEldc_int(k,NULL));
    }
  } else if (is_ldc_int(*c,&x) &&
      is_ldc_int(next(*c),&y)) {
    if (is_iadd(nextby(*c,2))) {
      k = x+y;
    } else if (is_isub(nextby(*c,2))) {
      k = x-y;
    } else if (is_imul(nextby(*c,2))) {
      k = x*y;
    } else if (is_idiv(nextby(*c,2))) {
      k = x/y;
    } else if (is_irem(nextby(*c,2))) {
      k = x%y;
    }
    if (k >= 0) {
      return replace(c,3,makeCODEldc_int(k,NULL));
    }
  }
  return 0;
}

/* iload x        iload x     ldc 0
 * ldc 0          ldc 1       iload x
 * iadd / isub    idiv        iadd
 * ------>        ------>     ------>
 * iload x        iload x     iload x
 */
int simplify_trivial_op(CODE **c)
{ int x,k;
  if (is_iload(*c,&x) &&
      is_ldc_int(next(*c),&k)) {
    if (k == 0) {
      if (is_iadd(nextby(*c,2)) ||
          is_isub(nextby(*c,2))) {
        return replace(c,3,makeCODEiload(x,NULL));
      }
    } else if (k == 1) {
      if (is_idiv(nextby(*c,2))) {
        return replace(c,3,makeCODEiload(x,NULL));
      }
    }
  } else if (is_ldc_int(*c,&k) &&
             is_iload(next(*c),&x) &&
             is_iadd(nextby(*c,2)) &&
             k == 0) {
    return replace(c,3,makeCODEiload(x,NULL));
  }
  return 0;
}

/* nop
 * --------->
 * 
 */ 
int remove_nop(CODE **c)
{ 
  if (is_nop(*c)) {
    return kill_line(c);
  }
  return 0;
}

/* dup
 * pop
 * --------->
 * 
 */
int remove_dup_pop(CODE **c)
{ 
  if (is_dup(*c) &&
      is_pop(next(*c))) {
    return replace_modified(c,2,NULL);
  }
  return 0;
}

/* swap
 * swap
 * --------->
 * 
 */
int remove_2_swap(CODE **c)
{ 
  if (is_swap(*c) &&
      is_swap(next(*c))) {
    return replace_modified(c,2,NULL);
  }
  return 0;
}

/* aload x
 * astore x
 * --------->
 * 
 */
int remove_aload_astore(CODE **c)
{ int x1, x2;
  if (is_aload(*c,&x1) &&
      is_astore(next(*c),&x2) &&
      x1 == x2) {
    return replace_modified(c,2,NULL);
  }
  return 0;
}

/* iload x
 * istore x
 * --------->
 * 
 */
int remove_iload_istore(CODE **c)
{ int x1,x2;
  if (is_iload(*c,&x1) &&
      is_istore(next(*c),&x2) &&
      x1 == x2) {
    return replace_modified(c,2,NULL);
  }
  return 0;
}

/* L:    (with no incoming edges)
 * --------->
 * 
 */
int remove_deadlabel(CODE **c) {
  int l;
  if(is_label(*c,&l) &&
     deadlabel(l)) {
    return kill_line(c);
  }
  return 0;
}



/* also do this for astore and for _k (does this save memory?)
 * istore k
 * iload m
 * k = m
 * ------>
 * dup
 * istore k
 */

/* branching */
/* 
 * iload_0
 * ifeq L
 * --------->
 * goto L
 */

/* L1:
 * L2:
 * ------>
 * L1:    (replace all references to L2 with L1)
 */

/* 
 * iload k (or iload_k, where k != 0)
 * ifne L
 * --------->
 * goto L
 */

/* isub
 * ifeq L
 * --------->
 * if_icmpeq L
 */ 

/* isub
 * ifne L
 * --------->
 * if_icmpne L
 */ 

/* aconst_null
 * ifnull L
 * --------->
 * goto L
 */ 

/* astore k
 * getfield ...
 * astore k
 * --------->
 * astore k
 * dup
 * getfield ...
 */ 

/* repeat this for loads (but with swap at the end) and with istores and iloads
 * swap
 * astore k
 * astore m
 * --------->
 * astore m
 * astore k
 */ 

/* THIS MIGHT NOT BE SOUND, ONLY IMPLEMENT THIS IF DUP+SWAP < ALOAD K
 * astore k
 * aload m
 * aload k
 * --------->
 * dup
 * astore k
 * aload m
 * swap
 */ 

/*
// loop folding?
// put in loop invariants? (VMs slide 60)
// simplify arithmetic expressions that have constants?
// factor arithmetic expressions?
// simplify control flow to better optimize?
// write a program to test out the permutations of our rules to get the best order of optimizations?
// swap order arithmetic expressions because it might allow for optimizations? This would remove the need for the following symmetric patterns but doesn't follow a lexicographic order. If we put the swap at the beginning though, it might find an optimizing rule and that that iteration will decrease the lexicographic order.
*/
/* 
 * 
 * --------->
 * 
 */ 

/* would this change anything (with an empty stack)?
 * pop
 * dup
 * --------->
 */



/****** Old style - still works, but better to use new style.
#define OPTS 4

OPTI optimization[OPTS] = {simplify_multiplication_right,
                           simplify_astore,
                           positive_increment,
                           simplify_goto_goto};
********/

/* new style for giving patterns */

int init_patterns()
{ ADD_PATTERN(simplify_multiplication_right);
  ADD_PATTERN(simplify_astore);
  ADD_PATTERN(positive_increment);
  ADD_PATTERN(simplify_goto_goto);
  ADD_PATTERN(simplify_multiplication_left);
  ADD_PATTERN(simplify_istore);
  ADD_PATTERN(simplify_aload);
  ADD_PATTERN(simplify_iload);
  ADD_PATTERN(simplify_aload_swap_putfield);
  ADD_PATTERN(simplify_constant_op);
  ADD_PATTERN(simplify_trivial_op);
  ADD_PATTERN(remove_nop);
  ADD_PATTERN(remove_dup_pop);
  ADD_PATTERN(remove_2_swap);
  ADD_PATTERN(remove_aload_astore);
  ADD_PATTERN(remove_iload_istore);
  ADD_PATTERN(remove_deadlabel);
  return 1;
}
