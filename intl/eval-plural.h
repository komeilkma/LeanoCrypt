
#ifndef STATIC
#define STATIC static
#endif

/* Evaluate the plural expression and return an index value.  */
STATIC
unsigned long int
internal_function
plural_eval (struct expression *pexp, unsigned long int n)
{
  switch (pexp->nargs)
    {
    case 0:
      switch (pexp->operation)
	{
	case var:
	  return n;
	case num:
	  return pexp->val.num;
	default:
	  break;
	}
      /* NOTREACHED */
      break;
    case 1:
      {
	/* pexp->operation must be lnot.  */
	unsigned long int arg = plural_eval (pexp->val.args[0], n);
	return ! arg;
      }
    case 2:
      {
	unsigned long int leftarg = plural_eval (pexp->val.args[0], n);
	if (pexp->operation == lor)
	  return leftarg || plural_eval (pexp->val.args[1], n);
	else if (pexp->operation == land)
	  return leftarg && plural_eval (pexp->val.args[1], n);
	else
	  {
	    unsigned long int rightarg = plural_eval (pexp->val.args[1], n);

	    switch (pexp->operation)
	      {
	      case mult:
		return leftarg * rightarg;
	      case divide:
#if !INTDIV0_RAISES_SIGFPE
		if (rightarg == 0)
		  raise (SIGFPE);
#endif
		return leftarg / rightarg;
	      case module:
#if !INTDIV0_RAISES_SIGFPE
		if (rightarg == 0)
		  raise (SIGFPE);
#endif
		return leftarg % rightarg;
	      case plus:
		return leftarg + rightarg;
	      case minus:
		return leftarg - rightarg;
	      case less_than:
		return leftarg < rightarg;
	      case greater_than:
		return leftarg > rightarg;
	      case less_or_equal:
		return leftarg <= rightarg;
	      case greater_or_equal:
		return leftarg >= rightarg;
	      case equal:
		return leftarg == rightarg;
	      case not_equal:
		return leftarg != rightarg;
	      default:
		break;
	      }
	  }
	/* NOTREACHED */
	break;
      }
    case 3:
      {
	/* pexp->operation must be qmop.  */
	unsigned long int boolarg = plural_eval (pexp->val.args[0], n);
	return plural_eval (pexp->val.args[boolarg ? 1 : 2], n);
      }
    }
  /* NOTREACHED */
  return 0;
}
