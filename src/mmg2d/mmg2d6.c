/* =============================================================================
**  This file is part of the mmg software package for the tetrahedral
**  mesh modification.
**  Copyright (c) Bx INP/Inria/UBordeaux/UPMC, 2004- .
**
**  mmg is free software: you can redistribute it and/or modify it
**  under the terms of the GNU Lesser General Public License as published
**  by the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  mmg is distributed in the hope that it will be useful, but WITHOUT
**  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
**  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
**  License for more details.
**
**  You should have received a copy of the GNU Lesser General Public
**  License and of the GNU General Public License along with mmg (in
**  files COPYING.LESSER and COPYING). If not, see
**  <http://www.gnu.org/licenses/>. Please read their terms carefully and
**  use this copy of the mmg distribution only if you accept them.
** =============================================================================
*/
/**
 * \file mmg2d/mmg2d6.c
 * \brief Isosurface discretization.
 * \author Charles Dapogny (UPMC)
 * \author Cécile Dobrzynski (Bx INP/Inria/UBordeaux)
 * \author Pascal Frey (UPMC)
 * \author Algiane Froehly (Inria/UBordeaux)
 * \version 5
 * \copyright GNU Lesser General Public License.
 */
#include "mmg2d.h"


/**
 * \param mesh pointer toward the mesh structure.
 * \param sol pointer toward the sol structure.
 * \return 1 if success.
 *
 * Isosurface discretization
 *
 **/

/* Check whether snapping the value of vertex i of k to 0 exactly leads to a non manifold situation 
 assumption: the triangle k has vertex i with value 0 and the other two with changing values */
int _MMG2_ismaniball(MMG5_pMesh mesh, MMG5_pSol sol, int start, char istart) {
  MMG5_pTria       pt;
  int              *adja,k,ip1,ip2,end1;
  char             i,i1,smsgn;
  
  k = start;
  i = _MMG5_inxt2[istart];
  
  /* First loop: stop if an external boundary, or a change in signs (or a 0) is met 
     recall that MS_SMGSGN(a,b) = 1 provided a*b >0 */
  do{
    adja = &mesh->adja[3*(k-1)+1];
    k = adja[i] / 3;
    i1 = adja[i] % 3;
    i = _MMG5_iprv2[i1];
    
    if ( k==0 ) break;
    
    pt = &mesh->tria[k];
    ip1 = pt->v[i1];
    ip2 = pt->v[i];
    
    smsgn = MS_SMSGN(sol->m[ip1],sol->m[ip2]) ? 1 : 0;
  }
  while ( smsgn );
  
  end1 = k;
  k = start;
  i = _MMG5_iprv2[istart];
  
  /* Second loop: same travel in the opposite sense */
  do{
    adja = &mesh->adja[3*(k-1)+1];
    k = adja[i] / 3;
    i1 = adja[i] % 3;
    i = _MMG5_inxt2[i1];
    
    if ( k==0 ) break;
    
    pt = &mesh->tria[k];
    ip1 = pt->v[i1];
    ip2 = pt->v[i];
    
    smsgn = MS_SMSGN(sol->m[ip1],sol->m[ip2]) ? 1 : 0;
  }
  while ( smsgn );
  
  /* If first stop was due to an external boundary, the second one must too; 
     else, the final triangle for the first travel must be that of the second one */
  if ( k != end1 ) {
    printf("triangle %d, point %d ; unsnap \n",start,mesh->tria[start].v[istart]);
    return(0);
  }
  return(1);
}

/* Snap values of sol very close to 0 to 0 exactly (to avoid very small triangles in cutting) */
int _MMG2_snapval(MMG5_pMesh mesh, MMG5_pSol sol, double *tmp) {
  MMG5_pTria       pt;
  MMG5_pPoint      p0;
  int              k,ns,nc,ip,ip1,ip2;
  char             i;
  
  /* Reset point flags */
  for (k=1; k<=mesh->np; k++)
    mesh->point[k].flag = 0;
  
  /* Snap values of sol that are close to 0 to 0 exactly */
  ns = nc = 0;
  for (k=1; k<=mesh->np; k++) {
    p0 = &mesh->point[k];
    if ( !MG_VOK(p0) ) continue;
    if ( fabs(sol->m[k]) < _MMG5_EPS ) {
      tmp[k] = ( fabs(sol->m[k]) < _MMG5_EPSD ) ? (-100.0*_MMG5_EPS) : sol->m[k];
      p0->flag = 1;
      sol->m[k] = 0.0;
      ns++;
    }
  }
  
  /* Check that the snapping process has not led to a nonmanifold situation */
  /* TO DO: Check first that every snapped point corresponds to a change in signs */
  
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) ) continue;
    for (i=0; i<3; i++) {
      ip = pt->v[i];
      ip1 = pt->v[_MMG5_inxt2[i]];
      ip2 = pt->v[_MMG5_iprv2[i]];
      
      p0 = &mesh->point[ip];
      /* Catch a snapped point by a triangle where there is a sign change */
      if ( p0->flag && !(MS_SMSGN(sol->m[ip1],sol->m[ip2])) ) {
        if ( !_MMG2_ismaniball(mesh,sol,k,i) ) {
          sol->m[ip] = tmp[ip];
          nc++;
        }
        p0->flag = 0;
        tmp[ip] = 0.0;
      }
    }
  }
  
  if ( (abs(mesh->info.imprim) > 5 || mesh->info.ddebug) && ns+nc > 0 )
    fprintf(stdout,"     %8d points snapped, %d corrected\n",ns,nc);
  
  return(1);
}

/* Check whether the ball of vertex i in tria start is manifold;
 by assumption, i inxt[i] is one edge of the implicit boundary */
int _MMG2_chkmaniball(MMG5_pMesh mesh, int start, char istart) {
  MMG5_pTria         pt;
  int                *adja,k,iel,refstart;
  char               i,i1;
  
  pt = &mesh->tria[start];
  k = start;
  i = istart;
  refstart = pt->ref;
  
  /* First travel, while another part of the implicit boundary is not met */
  do {
    adja = &mesh->adja[3*(k-1)+1];
    i1 = _MMG5_inxt2[i];
    
    k = adja[i1] / 3;
    i = adja[i1] % 3;
    i = _MMG5_inxt2[i];
  }
  while ( k && ( mesh->tria[k].ref == refstart ) );
  
  /* Case where a boundary is hit: travel in the other sense from start, and make sure
   that a boundary is hit too */
  if ( k == 0 ) {
    k = start;
    i = istart;

    adja = &mesh->adja[3*(k-1)+1];
    i1 = _MMG5_iprv2[i];
    k = adja[i1] / 3;
    i = adja[i1] % 3;
    i = _MMG5_iprv2[i];
    
    /* Check of the way the point is caught (the left-hand edge is not an external edge) */
    assert ( k );
    
    do {
      adja = &mesh->adja[3*(k-1)+1];
      i1 = _MMG5_iprv2[i];
      
      k = adja[i1] / 3;
      i = adja[i1] % 3;
      i = _MMG5_iprv2[i];
    }
    while ( k && ( mesh->tria[k].ref != refstart ) );
    
    if ( k == 0 ) return(1);
    else          return(0);
      
  }
  
  /* General case: go on travelling until another implicit boundary is met */
  do {
    adja = &mesh->adja[3*(k-1)+1];
    i1 = _MMG5_inxt2[i];
    
    k = adja[i1] / 3;
    i = adja[i1] % 3;
    i = _MMG5_inxt2[i];
  }
  while ( k && ( mesh->tria[k].ref != refstart ) );
  
  /* At least 3 boundary segments meeting at p */
  if ( k != start )
    return(0);
  
  return(1);
}

/* Check whether the resulting two subdomains occupying mesh are manifold */
int _MMG2_chkmanimesh(MMG5_pMesh mesh) {
  MMG5_pTria      pt,pt1;
  int             *adja,k,cnt,iel;
  char            i,i1;
  
  
  /* First check: check whether one triangle in the mesh has 3 boundary faces */
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) ) continue;
    
    adja = &mesh->adja[3*(k-1)+1];
    for (i=0; i<3; i++) {
      iel = adja[i] / 3;
      
      if (!iel ) {
        cnt++;
        continue;
      }
      else {
        pt1 = &mesh->tria[iel];
        if ( pt1->ref != pt->ref ) cnt++;
      }
    }
    if( cnt == 3 ) {
      printf("Triangle %d: 3 boundary edges\n",k);
      return(0);
    }
  }
  
  /* Second check: check whether the configuration is manifold in the ball of each point;
     each vertex on the implicit boundary is caught in such a way that i1 inxt[i1] is one edge of the implicit 
     boundary */
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) ) continue;
    
    for (i=0; i<3; i++) {
      adja = &mesh->adja[3*(k-1)+1];
      iel = adja[i] / 3;
      
      if (! iel ) continue;
      pt1 = &mesh->tria[iel];
      if ( pt->ref == pt1->ref ) continue;
      
      i1 = _MMG5_inxt2[i];
      if ( !_MMG2_chkmaniball(mesh,k,i1) )
        return(0);
    }
  }
  
  if ( mesh->info.imprim || mesh->info.ddebug )
    fprintf(stdout,"  *** Manifold implicit surface.\n");
  return(1);
}

/* Effective discretization of the 0 level set encoded in sol in the mesh */
int _MMG2_cuttri_ls(MMG5_pMesh mesh, MMG5_pSol sol){
  MMG5_pTria   pt;
  MMG5_pPoint  p0,p1;
  _MMG5_Hash   hash;
  double       v0,v1,s,c[2];
  int          k,ip0,ip1,nb,np,nt,ns,vx[3];
  char         i,i0,i1;
  
  /* Reset flag field for points */
  for (k=1; k<=mesh->np; k++)
    mesh->point[k].flag = 0;
  
  /* Evaluate the number of intersected edges by the 0 level set */
  nb = 0;
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) ) continue;
    
    for (i=0; i<3; i++) {
      i0 = _MMG5_inxt2[i];
      i1 = _MMG5_inxt2[i0];
      
      ip0 = pt->v[i0];
      ip1 = pt->v[i1];
      
      p0 = &mesh->point[ip0];
      p1 = &mesh->point[ip1];
      
      if ( p0->flag && p1->flag ) continue;
      
      v0 = sol->m[ip0];
      v1 = sol->m[ip1];
      
      if ( fabs(v0) > _MMG5_EPSD2 && fabs(v1) > _MMG5_EPSD2 && v0*v1 < 0.0 ) {
        nb++;
        if ( !p0->flag ) p0->flag = nb;
        if ( !p1->flag ) p1->flag = nb;
      }
    }
  }
  if ( !nb ) return(1);
  
  /* Create the intersection points between the edges in the mesh and the 0 level set */
  if ( !_MMG5_hashNew(mesh,&hash,nb,2*nb) ) return(0);
  
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) ) continue;
    
    for (i=0; i<3; i++) {
      i0 = _MMG5_inxt2[i];
      i1 = _MMG5_inxt2[i0];
      
      ip0 = pt->v[i0];
      ip1 = pt->v[i1];
      
      p0 = &mesh->point[ip0];
      p1 = &mesh->point[ip1];
      
      np = _MMG5_hashGet(&hash,ip0,ip1);
      if ( np ) continue;
      
      v0 = sol->m[ip0];
      v1 = sol->m[ip1];
      
      if ( fabs(v0) < _MMG5_EPSD2 || fabs(v1) < _MMG5_EPSD2 )  continue;
      else if ( MS_SMSGN(v0,v1) )  continue;
      else if ( !p0->flag || !p1->flag )  continue;
      
      /* Intersection point between edge p0p1 and the 0 level set */
      s = v0/(v0-v1);
      s = MG_MAX(MG_MIN(s,1.0-_MMG5_EPS),_MMG5_EPS);
      
      c[0] = p0->c[0] + s*(p1->c[0]-p0->c[0]);
      c[1] = p0->c[1] + s*(p1->c[1]-p0->c[1]);
      
      np = _MMG2D_newPt(mesh,c,0);
      if ( !np ) {
        printf("*** Insufficient memory; abort\n");
        return(0);
      }
      sol->m[np] = 0.0;
      _MMG5_hashEdge(mesh,&hash,ip0,ip1,np);
    }
  }
  
  /* Proceed to splitting by calling patterns */
  nt = mesh->nt;
  ns = 0;
  
  for (k=1; k<=nt; k++) {
    
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) ) continue;
    pt->flag = 0;
    
    for (i=0; i<3; i++) {
      i0 = _MMG5_inxt2[i];
      i1 = _MMG5_inxt2[i0];
      
      ip0 = pt->v[i0];
      ip1 = pt->v[i1];
      
      vx[i] = _MMG5_hashGet(&hash,ip0,ip1);
      
      if ( vx[i] ) MG_SET(pt->flag,i);
    }
    
    switch( pt->flag ) {
      /* 1 edge split -> 0-+ */
      case 1: case 2: case 4:
        _MMG2_split1(mesh,sol,k,vx);
        ns++;
        break;
      
      /* 2 edge split -> +-- or -++ */
      case 3: case 5: case 6:
        _MMG2_split2(mesh,sol,k,vx);
        ns++;
        break;
    
      default:
        assert(pt->flag==0);
        break;
    }
  }
  
  if ( (mesh->info.ddebug || abs(mesh->info.imprim) > 5) && ns > 0 )
    fprintf(stdout,"     %7d splitted\n",ns);
  
  _MMG5_DEL_MEM(mesh,hash.item,(hash.max+1)*sizeof(_MMG5_hedge));
  return(ns);
  
}

/* Set references to the new triangles */
int _MMG2_setref_ls(MMG5_pMesh mesh, MMG5_pSol sol){
  MMG5_pTria    pt;
  int           k,ip;
  char          i,nmn,npl,nz;
  
  for (k=1; k<=mesh->nt; k++) {
    pt = &mesh->tria[k];
    if ( !MG_EOK(pt) ) continue;
    
    nmn = npl = nz = 0;
    for (i=0; i<3; i++) {
      ip = pt->v[i];
      
      if ( sol->m[ip] > 0.0 )
        npl++;
      else if ( sol->m[ip] < 0.0 )
        nmn++;
      else
        nz++;
    }
    
    assert(nz < 3);
    if ( npl ) {
      assert ( !nmn );
      pt->ref = MS_PLUS;
    }
    else {
      assert ( !npl );
      pt->ref = MS_MINUS;
    }
  }
  
  return(1);
}

/* Main function of the -ls mode */
int MMG2_mmg2d6(MMG5_pMesh mesh, MMG5_pSol sol) {
  double *tmp;
  
  if ( abs(mesh->info.imprim) > 3 )
    fprintf(stdout,"  ** ISOSURFACE EXTRACTION\n");
  
  /* Allocate memory for tmp */
  _MMG5_ADD_MEM(mesh,(mesh->npmax+1)*sizeof(double),"temporary table",
                printf("  Exit program.\n");
                exit(EXIT_FAILURE));
  _MMG5_SAFE_CALLOC(tmp,mesh->npmax+1,double);
  
  /* Snap values of the level set function which are very close to 0 to 0 exactly */
  if ( !_MMG2_snapval(mesh,sol,tmp) ) {
    fprintf(stdout,"  ## Wrong input implicit function. Exit program.\n");
    return(0);
  }
  
  _MMG5_DEL_MEM(mesh,tmp,(mesh->npmax+1)*sizeof(double));
  
  /* Creation of adjacency relations in the mesh */
  if ( !MMG2_hashTria(mesh) ) {
    fprintf(stdout,"  ## Hashing problem. Exit program.\n");
    return(0);
  }
  
  /* No need to keep adjacencies from now on */
  _MMG5_DEL_MEM(mesh,mesh->adja,(3*mesh->ntmax+5)*sizeof(int));
  
  /* Transfer the boundary edge references to the triangles */
  if ( !MMG2_assignEdge(mesh) ) {
    fprintf(stdout,"  ## Problem in setting boundary. Exit program.\n");
    return(0);
  }
  
  /* Effective splitting of the crossed triangles */
  if ( !_MMG2_cuttri_ls(mesh,sol) ) {
    fprintf(stdout,"  ## Problem in cutting triangles. Exit program.\n");
    return(0);
  }
  
  /* Set references on the interior / exterior triangles*/
  if ( !_MMG2_setref_ls(mesh,sol) ) {
    fprintf(stdout,"  ## Problem in setting references. Exit program.\n");
    return(0);
  }
  
  /* Creation of adjacency relations in the mesh */
  if ( !MMG2_hashTria(mesh) ) {
    fprintf(stdout,"  ## Hashing problem. Exit program.\n");
    return(0);
  }
  
  /* Check that the resulting mesh is manifold */
  if ( !_MMG2_chkmanimesh(mesh) ) {
    fprintf(stdout,"  ## No manifold resulting situation. Exit program.\n");
    return(0);
  }
  
  /* Clean memory */
  _MMG5_DEL_MEM(mesh,sol->m,(sol->size*(sol->npmax+1))*sizeof(double));
  
  return(1);
}

