/* 
 * This is a modified version of the High Performance Computing Linpack
 * Benchmark (HPL). All code not contained in the original HPL version
 * 2.0 is property of the Frankfurt Institute for Advanced Studies
 * (FIAS). None of the material may be copied, reproduced, distributed,
 * republished, downloaded, displayed, posted or transmitted in any form
 * or by any means, including, but not limited to, electronic,
 * mechanical, photocopying, recording, or otherwise, without the prior
 * written permission of FIAS. For those parts contained in the
 * unmodified version of the HPL the below copyright notice applies.
 * 
 * Authors:
 * David Rohr (drohr@jwdt.org)
 * Matthias Bach (bach@compeng.uni-frankfurt.de)
 * Matthias Kretz (kretz@compeng.uni-frankfurt.de)
 * 
 * -- High Performance Computing Linpack Benchmark (HPL)                
 *    HPL - 2.0 - September 10, 2008                          
 *    Antoine P. Petitet                                                
 *    University of Tennessee, Knoxville                                
 *    Innovative Computing Laboratory                                 
 *    (C) Copyright 2000-2008 All Rights Reserved                       
 *                                                                      
 * -- Copyright notice and Licensing terms:                             
 *                                                                      
 * Redistribution  and  use in  source and binary forms, with or without
 * modification, are  permitted provided  that the following  conditions
 * are met:                                                             
 *                                                                      
 * 1. Redistributions  of  source  code  must retain the above copyright
 * notice, this list of conditions and the following disclaimer.        
 *                                                                      
 * 2. Redistributions in binary form must reproduce  the above copyright
 * notice, this list of conditions,  and the following disclaimer in the
 * documentation and/or other materials provided with the distribution. 
 *                                                                      
 * 3. All  advertising  materials  mentioning  features  or  use of this
 * software must display the following acknowledgement:                 
 * This  product  includes  software  developed  at  the  University  of
 * Tennessee, Knoxville, Innovative Computing Laboratory.             
 *                                                                      
 * 4. The name of the  University,  the name of the  Laboratory,  or the
 * names  of  its  contributors  may  not  be used to endorse or promote
 * products  derived   from   this  software  without  specific  written
 * permission.                                                          
 *                                                                      
 * -- Disclaimer:                                                       
 *                                                                      
 * THIS  SOFTWARE  IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,  INCLUDING,  BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE UNIVERSITY
 * OR  CONTRIBUTORS  BE  LIABLE FOR ANY  DIRECT,  INDIRECT,  INCIDENTAL,
 * SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES  (INCLUDING,  BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA OR PROFITS; OR BUSINESS INTERRUPTION)  HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT,  STRICT LIABILITY,  OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 * ---------------------------------------------------------------------
 */ 
/*
 * Include files
 */
#include "hpl.h"

#ifdef HPL_NO_MPI_DATATYPE  /* The user insists to not use MPI types */
#ifndef HPL_COPY_L       /* and also want to avoid the copy of L ... */
#define HPL_COPY_L   /* well, sorry, can not do that: force the copy */
#endif
#endif

#ifdef STDC_HEADERS
int HPL_binit_mpi
(
   HPL_T_panel *              PANEL
)
#else
int HPL_binit_mpi( PANEL )
   HPL_T_panel *              PANEL;
#endif
{
#ifdef HPL_USE_MPI_DATATYPE
/*
 * .. Local Variables ..
 */
   int                        ierr;
#endif
/* ..
 * .. Executable Statements ..
 */
   if( PANEL == NULL )           { return( HPL_SUCCESS ); }
   if( PANEL->grid->npcol <= 1 ) { return( HPL_SUCCESS ); }
#ifdef HPL_USE_MPI_DATATYPE
#ifdef HPL_COPY_L
/*
 * Copy the panel into a contiguous buffer
 */
   HPL_copyL( PANEL );
#endif
/*
 * Create the MPI user-defined data type
 */
   ierr = HPL_packL( PANEL, 0, PANEL->len, 0 );
 
   return( ( ierr == MPI_SUCCESS ? HPL_SUCCESS : HPL_FAILURE ) );
#else
/*
 * Force the copy of the panel into a contiguous buffer
 */
   HPL_copyL( PANEL );

   return( HPL_SUCCESS );
#endif
}

#ifdef HPL_USE_MPI_DATATYPE

#define   _M_BUFF     PANEL->buffers[0]
#define   _M_COUNT    PANEL->counts[0]
#define   _M_TYPE     PANEL->dtypes[0]

#else

#define   _M_BUFF     (void *)(PANEL->L2)
#define   _M_COUNT    PANEL->len
#define   _M_TYPE     MPI_DOUBLE

#endif

#ifdef STDC_HEADERS
int HPL_bcast_mpi
(
   HPL_T_panel                * PANEL,
   int                        * IFLAG
)
#else
int HPL_bcast_mpi( PANEL, IFLAG )
   HPL_T_panel                * PANEL;
   int                        * IFLAG;
#endif
{
/*
 * .. Local Variables ..
 */
   MPI_Comm                   comm;
   int                        ierr, go, next, msgid, prev, rank, root,
                              size;
/* ..
 * .. Executable Statements ..
 */
   if( PANEL == NULL ) { *IFLAG = HPL_SUCCESS; return( HPL_SUCCESS ); }
   if( ( size = PANEL->grid->npcol ) <= 1 )
   {                     *IFLAG = HPL_SUCCESS; return( HPL_SUCCESS ); }
/*
 * Cast phase:  If I am the root process, start spreading the panel.  If
 * I am not the root process, probe for message. If the message is here,
 * then receive it, and  if I am not the last process of the ring, then
 * forward it to the next.  Otherwise, inform the caller that the panel
 * has still not been received.
 */
   rank = PANEL->grid->mycol; comm  = PANEL->grid->row_comm;
   root = PANEL->pcol;        msgid = PANEL->msgid;
   size = PANEL->grid->npcol;

checkMpiThread    ierr = MPI_Bcast( _M_BUFF, _M_COUNT, _M_TYPE, root, comm );

/*
 * If the message was received and being forwarded,  return HPL_SUCCESS.
 * If an error occured in an MPI call, return HPL_FAILURE.
 */  
   *IFLAG = ( ierr == MPI_SUCCESS ? HPL_SUCCESS : HPL_FAILURE );

   return( *IFLAG );
}

#ifdef STDC_HEADERS
int HPL_bwait_mpi
(
   HPL_T_panel *              PANEL
)
#else
int HPL_bwait_mpi( PANEL )
   HPL_T_panel *              PANEL;
#endif
{
#ifdef HPL_USE_MPI_DATATYPE
/*
 * .. Local Variables ..
 */
   int                        ierr;
#endif
/* ..
 * .. Executable Statements ..
 */
   if( PANEL == NULL )           { return( HPL_SUCCESS ); }
   if( PANEL->grid->npcol <= 1 ) { return( HPL_SUCCESS ); }
/*
 * Release the arrays of request / status / data-types and buffers 
 */
#ifdef HPL_USE_MPI_DATATYPE
checkMpiThread    ierr = MPI_Type_free( &PANEL->dtypes[0] );
   return( ( ierr == MPI_SUCCESS ? HPL_SUCCESS : HPL_FAILURE ) );
#else
   return( HPL_SUCCESS );
#endif
}
