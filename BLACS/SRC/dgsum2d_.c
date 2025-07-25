#include "Bdef.h"


#if (INTFACE == C_CALL)
void Cdgsum2d(Int ConTxt, char *scope, char *top, Int m, Int n, double *A,
              Int lda, Int rdest, Int cdest)
#else
F_VOID_FUNC dgsum2d_(Int *ConTxt, F_CHAR scope, F_CHAR top, Int *m, Int *n,
                     double *A, Int *lda, Int *rdest, Int *cdest)
#endif
/*
 *  -- V1.1 BLACS routine --
 *  University of Tennessee, May 1, 1996
 *  Written by Clint Whaley.
 *
 *  Purpose
 *  =======
 *  Combine sum operation for double precision rectangular matrices.
 *
 *  Arguments
 *  =========
 *
 *  ConTxt  (input) Ptr to Int
 *          Index into MyConTxts00 (my contexts array).
 *
 *  SCOPE   (input) Ptr to char
 *          Limit the scope of the operation.
 *          = 'R' :   Operation is performed by a process row.
 *          = 'C' :   Operation is performed by a process column.
 *          = 'A' :   Operation is performed by all processes in grid.
 *
 *  TOP     (input) Ptr to char
 *          Controls fashion in which messages flow within the operation.
 *
 *  M       (input) Ptr to Int
 *          The number of rows of the matrix A.  M >= 0.
 *
 *  N       (input) Ptr to Int
 *          The number of columns of the matrix A.  N >= 0.
 *
 *  A       (output) Ptr to double precision two dimensional array
 *          The m by n matrix A.  Fortran77 (column-major) storage
 *          assumed.
 *
 *  LDA     (input) Ptr to Int
 *          The leading dimension of the array A.  LDA >= M.
 *
 *  RDEST   (input) Ptr to Int
 *          The process row of the destination of the sum.
 *          If rdest == -1, then result is left on all processes in scope.
 *
 *  CDEST   (input) Ptr to Int
 *          The process column of the destination of the sum.
 *          If rdest == -1, then CDEST ignored.
 *
 * ------------------------------------------------------------------------
 */
{
   void BI_ArgCheck(Int, Int, char *, char, char, char, Int, Int, Int, Int,
                    Int *, Int *);
   void BI_UpdateBuffs(BLACBUFF *);
   BLACBUFF *BI_GetBuff(Int);
   Int BI_BuffIsFree(BLACBUFF *, Int);
   MPI_Datatype BI_GetMpiGeType(BLACSCONTEXT *, Int, Int, Int,
                                   MPI_Datatype, Int *);
   BLACBUFF *BI_Pack(BLACSCONTEXT *, BVOID *, BLACBUFF *, MPI_Datatype);
   MPI_Datatype BI_GetMpiGeType(BLACSCONTEXT *, Int, Int, Int,
                                   MPI_Datatype, Int *);
   void BI_Unpack(BLACSCONTEXT *, BVOID *, BLACBUFF *, MPI_Datatype);
   void BI_MringComb(BLACSCONTEXT *, BLACBUFF *, BLACBUFF *, Int, VVFUNPTR,
                     Int, Int);
   void BI_TreeComb(BLACSCONTEXT *, BLACBUFF *, BLACBUFF *, Int, VVFUNPTR,
                    Int, Int);
   void BI_BeComb(BLACSCONTEXT *, BLACBUFF *, BLACBUFF *, Int, VVFUNPTR);
   void BI_dvvsum(Int, char *, char *);
/*
 *  Variable Declarations
 */
   BLACBUFF *bp, *bp2;
   BLACSCONTEXT *ctxt;
   char ttop, tscope;
   Int N, length, dest, tlda, trdest, ierr;
   extern BLACBUFF *BI_ActiveQ;
   extern BLACBUFF BI_AuxBuff;

   MGetConTxt(Mpval(ConTxt), ctxt);
   ttop = F2C_CharTrans(top);
   ttop = Mlowcase(ttop);
   tscope = F2C_CharTrans(scope);
   tscope = Mlowcase(tscope);
/*
 *  If the user has set the default combine topology, use it instead of
 *  BLACS default
 */
#ifdef DefCombTop
   if (ttop == ' ') ttop = DefCombTop;
#endif
   if (Mpval(cdest) == -1) trdest = -1;
   else trdest = Mpval(rdest);
#if (BlacsDebugLvl > 0)
   BI_ArgCheck(Mpval(ConTxt), RT_COMB, __FILE__, tscope, 'u', 'u', Mpval(m),
               Mpval(n), Mpval(lda), 1, &trdest, Mpaddress(cdest));
#endif
   if (Mpval(lda) >= Mpval(m)) tlda = Mpval(lda);
   else tlda = Mpval(m);
   switch(tscope)
   {
   case 'r':
      ctxt->scp = &ctxt->rscp;
      if (trdest == -1) dest = -1;
      else dest = Mpval(cdest);
      break;
   case 'c':
      ctxt->scp = &ctxt->cscp;
      dest = trdest;
      break;
   case 'a':
      ctxt->scp = &ctxt->ascp;
      if (trdest == -1) dest = -1;
      else dest = Mvkpnum(ctxt, trdest, Mpval(cdest));
      break;
   default:
      BI_BlacsErr(Mpval(ConTxt), __LINE__, __FILE__, "Unknown scope '%c'",
                  tscope);
   }


/*
 * It's not defined how MPI reacts to 0 element reductions, so use BLACS 1-tree
 * topology if we've got one.  Also, we can't use MPI functions if we need to
 * guarantee repeatability.
 */
   if (ttop == ' ')
      if ( (Mpval(m) < 1) || (Mpval(n) < 1) || (ctxt->TopsRepeat) ) ttop = '1';
   N = Mpval(m) * Mpval(n);
   length = N * sizeof(double);
/*
 * If A is contiguous, we can use it as one of the buffers
 */
   if ( (Mpval(m) == tlda) || (Mpval(n) == 1) )
   {
      bp = &BI_AuxBuff;
      bp->Buff = (char *) A;
      bp2 = BI_GetBuff(length);
   }
/*
 * Otherwise, we must allocate both buffers
 */
   else
   {
      bp = BI_GetBuff(length*2);
      bp2 = &BI_AuxBuff;
      bp2->Buff = &bp->Buff[length];
      BI_dmvcopy(Mpval(m), Mpval(n), A, tlda, (double*)bp->Buff);
   }
   bp->dtype = bp2->dtype = MPI_DOUBLE;
   bp->N = bp2->N = N;

   switch(ttop)
   {
   case ' ':         /* use MPI's reduction by default */
      if (dest != -1)
      {
         ierr=MPI_Reduce(bp->Buff, bp2->Buff, bp->N, bp->dtype, MPI_SUM,
                       dest, ctxt->scp->comm);
         if (ctxt->scp->Iam == dest)
	    BI_dvmcopy(Mpval(m), Mpval(n), A, tlda, (double*)bp2->Buff);
      }
      else
      {
         ierr=MPI_Allreduce(bp->Buff, bp2->Buff, bp->N, bp->dtype, MPI_SUM,
		          ctxt->scp->comm);
	 BI_dvmcopy(Mpval(m), Mpval(n), A, tlda, (double*)bp2->Buff);
      }
      if (BI_ActiveQ) BI_UpdateBuffs(NULL);
      return;
      break;
   case 'i':
      BI_MringComb(ctxt, bp, bp2, N, BI_dvvsum, dest, 1);
      break;
   case 'd':
      BI_MringComb(ctxt, bp, bp2, N, BI_dvvsum, dest, -1);
      break;
   case 's':
      BI_MringComb(ctxt, bp, bp2, N, BI_dvvsum, dest, 2);
      break;
   case 'm':
      BI_MringComb(ctxt, bp, bp2, N, BI_dvvsum, dest, ctxt->Nr_co);
      break;
   case '1':
   case '2':
   case '3':
   case '4':
   case '5':
   case '6':
   case '7':
   case '8':
   case '9':
      BI_TreeComb(ctxt, bp, bp2, N, BI_dvvsum, dest, ttop-47);
      break;
   case 'f':
      BI_TreeComb(ctxt, bp, bp2, N, BI_dvvsum, dest, FULLCON);
      break;
   case 't':
      BI_TreeComb(ctxt, bp, bp2, N, BI_dvvsum, dest, ctxt->Nb_co);
      break;
   case 'h':
/*
 *    Use bidirectional exchange if everyone wants answer
 */
      if ( (trdest == -1) && !(ctxt->TopsCohrnt) )
         BI_BeComb(ctxt, bp, bp2, N, BI_dvvsum);
      else
         BI_TreeComb(ctxt, bp, bp2, N, BI_dvvsum, dest, 2);
      break;
   default :
      BI_BlacsErr(Mpval(ConTxt), __LINE__, __FILE__, "Unknown topology '%c'",
                  ttop);
   }

/*
 * If I am selected to receive answer
 */
   if (bp != &BI_AuxBuff)
   {
      if ( (ctxt->scp->Iam == dest) || (dest == -1) )
         BI_dvmcopy(Mpval(m), Mpval(n), A, tlda, (double*)bp->Buff);
      BI_UpdateBuffs(bp);
   }
   else
   {
      if (BI_ActiveQ) BI_UpdateBuffs(NULL);
      BI_BuffIsFree(bp, 1);
   }
}
