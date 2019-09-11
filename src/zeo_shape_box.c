/* Zeo - Z/Geometry and optics computation library.
 * Copyright (C) 2005 Tomomichi Sugihara (Zhidao)
 *
 * zeo_shape_box - 3D shapes: box.
 */

#include <zeo/zeo_shape.h>

/* ********************************************************** */
/* CLASS: zBox3D
 * 3D box class
 * ********************************************************** */

static bool _zBox3DFScan(FILE *fp, void *instance, char *buf, bool *success);

/* create a 3D box. */
zBox3D *zBox3DCreate(zBox3D *box, zVec3D *c, zVec3D *ax, zVec3D *ay, zVec3D *az, double d, double w, double h)
{
  zBox3DSetCenter( box, c );
  zBox3DSetAxis( box, 0, ax );
  zBox3DSetAxis( box, 1, ay );
  zBox3DSetAxis( box, 2, az );
  zBox3DSetDepth( box, fabs(d) );
  zBox3DSetWidth( box, fabs(w) );
  zBox3DSetHeight( box, fabs(h) );
  return box;
}

/* initialize a 3D box. */
zBox3D *zBox3DInit(zBox3D *box)
{
  return zBox3DCreateAlign( box, ZVEC3DZERO, 0, 0, 0 );
}

/* allocate memory for a 3D box. */
zBox3D *zBox3DAlloc(void)
{
  zBox3D *box;
  if( !( box = zAlloc( zBox3D, 1 ) ) ){
    ZALLOCERROR();
    return NULL;
  }
  return box;
}

/* copy a 3D box to another. */
zBox3D *zBox3DCopy(zBox3D *src, zBox3D *dest)
{
  return zBox3DCreate( dest, zBox3DCenter(src),
    zBox3DAxis(src,0), zBox3DAxis(src,1), zBox3DAxis(src,2),
    zBox3DDepth(src), zBox3DWidth(src), zBox3DHeight(src) );
}

/* mirror a 3D box along an axis. */
zBox3D *zBox3DMirror(zBox3D *src, zBox3D *dest, zAxis axis)
{
  zBox3DCopy( src, dest );
  zBox3DCenter(dest)->e[axis] *= -1;
  zBox3DAxis(dest,0)->e[axis] *= -1;
  zBox3DAxis(dest,1)->e[axis] *= -1;
  zBox3DAxis(dest,2)->e[axis] *= -1;
  return dest;
}

/* transform coordinates of a 3D box. */
zBox3D *zBox3DXform(zBox3D *src, zFrame3D *f, zBox3D *dest)
{
  zXform3D( f, zBox3DCenter(src), zBox3DCenter(dest) );
  zMulMat3DVec3D( zFrame3DAtt(f), zBox3DAxis(src,zX), zBox3DAxis(dest,zX) );
  zMulMat3DVec3D( zFrame3DAtt(f), zBox3DAxis(src,zY), zBox3DAxis(dest,zY) );
  zMulMat3DVec3D( zFrame3DAtt(f), zBox3DAxis(src,zZ), zBox3DAxis(dest,zZ) );
  zBox3DSetDepth( dest, zBox3DDepth(src) );
  zBox3DSetWidth( dest, zBox3DWidth(src) );
  zBox3DSetHeight( dest, zBox3DHeight(src) );
  return dest;
}

/* inversely transform coordinates of a 3D box. */
zBox3D *zBox3DXformInv(zBox3D *src, zFrame3D *f, zBox3D *dest)
{
  zXform3DInv( f, zBox3DCenter(src), zBox3DCenter(dest) );
  zMulMat3DTVec3D( zFrame3DAtt(f), zBox3DAxis(src,zX), zBox3DAxis(dest,zX) );
  zMulMat3DTVec3D( zFrame3DAtt(f), zBox3DAxis(src,zY), zBox3DAxis(dest,zY) );
  zMulMat3DTVec3D( zFrame3DAtt(f), zBox3DAxis(src,zZ), zBox3DAxis(dest,zZ) );
  zBox3DSetDepth( dest, zBox3DDepth(src) );
  zBox3DSetWidth( dest, zBox3DWidth(src) );
  zBox3DSetHeight( dest, zBox3DHeight(src) );
  return dest;
}

/* the closest point from a 3D point to a 3D box. */
double zBox3DClosest(zBox3D *box, zVec3D *p, zVec3D *cp)
{
  zVec3D _p;
  double min, max;
  register zDir d;

  zXform3DInv( &box->f, p, &_p );
  for( d=zX; d<=zZ; d++ ){
    min =-0.5 * zBox3DDia(box,d);
    max = 0.5 * zBox3DDia(box,d);
    cp->e[d] = zLimit( _p.e[d], min, max );
  }
  zXform3DDRC( &box->f, cp );
  return zVec3DDist( p, cp );
}

/* distance from a point to a 3D box. */
double zBox3DPointDist(zBox3D *box, zVec3D *p)
{
  zVec3D cp;
  return zBox3DClosest( box, p, &cp );
}

/* check if a point is inside of a box. */
bool zBox3DPointIsInside(zBox3D *box, zVec3D *p, bool rim)
{
  register zDir d;
  zVec3D err;
  double l;

  zXform3DInv( &box->f, p, &err );
  for( d=zX; d<=zZ; d++ ){
    l = 0.5 * zBox3DDia(box,d);
    if( rim ) l += zTOL;
    if( err.e[d] > l || err.e[d] < -l )
      return false;
  }
  return true;
}

/* volume of a 3D box. */
double zBox3DVolume(zBox3D *box)
{
  return zBox3DDepth(box) * zBox3DWidth(box) * zBox3DHeight(box);
}

/* inertia of a 3D box. */
zMat3D *zBox3DInertia(zBox3D *box, zMat3D *inertia)
{
  zMat3D i;
  double xx, yy, zz, c;

  c = zBox3DVolume( box ) / 12;
  xx = zSqr( zBox3DDepth(box) ) * c;
  yy = zSqr( zBox3DWidth(box) ) * c;
  zz = zSqr( zBox3DHeight(box) ) * c;
  zMat3DCreate( &i,
    yy+zz, 0, 0,
    0, zz+xx, 0,
    0, 0, xx+yy );
  return zRotMat3D( zFrame3DAtt(&box->f), &i, inertia );
}

/* get vertex of a box. */
zVec3D *zBox3DVert(zBox3D *box, int i, zVec3D *v)
{
  _zVec3DCreate( v,
    ( (i&0x1)^(i>>1&0x1) ) ? -0.5*zBox3DDepth(box) : 0.5*zBox3DDepth(box),
    (  i&0x2             ) ? -0.5*zBox3DWidth(box) : 0.5*zBox3DWidth(box),
    (  i&0x4             ) ? -0.5*zBox3DHeight(box): 0.5*zBox3DHeight(box) );
  return zXform3DDRC( &box->f, v );
}

#define __zBox3DToPH_tri(p,i,i1,i2,i3,i4) do{\
  zTri3DCreate( zPH3DFace(p,i),   zPH3DVert(p,i1), zPH3DVert(p,i2), zPH3DVert(p,i3) );\
  zTri3DCreate( zPH3DFace(p,i+1), zPH3DVert(p,i1), zPH3DVert(p,i3), zPH3DVert(p,i4) );\
} while(0)

/* convert a box to a polyhedron. */
zPH3D* zBox3DToPH(zBox3D *box, zPH3D *ph)
{
  register int i;

  if( !zPH3DAlloc( ph, 8, 12 ) ) return NULL;
  for( i=0; i<8; i++ )
    zBox3DVert( box, i, zPH3DVert(ph,i) );
  __zBox3DToPH_tri( ph, 0, 0, 1, 2, 3 );
  __zBox3DToPH_tri( ph, 2, 0, 4, 5, 1 );
  __zBox3DToPH_tri( ph, 4, 1, 5, 6, 2 );
  __zBox3DToPH_tri( ph, 6, 2, 6, 7, 3 );
  __zBox3DToPH_tri( ph, 8, 3, 7, 4, 0 );
  __zBox3DToPH_tri( ph,10, 7, 6, 5, 4 );
  return ph;
}

/* parse ZTK format */

static zVec3D *_zBox3DAxisFromZTK(zBox3D *box, int i0, int i1, int i2, ZTK *ztk)
{
  if( ZTKValCmp( ztk, "auto" ) )
    zVec3DOuterProd( zBox3DAxis(box,i1), zBox3DAxis(box,i2), zBox3DAxis(box,i0) );
  else
    zVec3DFromZTK( zBox3DAxis(box,i0), ztk );
  zVec3DNormalizeDRC( zBox3DAxis(box,i0) );
  return zBox3DAxis(box,i0);
}

static void *_zBox3DCenterFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  return zVec3DFromZTK( zBox3DCenter((zBox3D*)obj), ztk ) ? obj : NULL; }
static void *_zBox3DAxisXFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  _zBox3DAxisFromZTK( (zBox3D*)obj, 0, 1, 2, ztk );
  return obj; }
static void *_zBox3DAxisYFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  _zBox3DAxisFromZTK( (zBox3D*)obj, 1, 2, 0, ztk );
  return obj; }
static void *_zBox3DAxisZFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  _zBox3DAxisFromZTK( (zBox3D*)obj, 2, 0, 1, ztk );
  return obj; }
static void *_zBox3DDepthFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  zBox3DDepth((zBox3D*)obj) = ZTKDouble(ztk);
  return obj; }
static void *_zBox3DWidthFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  zBox3DWidth((zBox3D*)obj) = ZTKDouble(ztk);
  return obj; }
static void *_zBox3DHeightFromZTK(void *obj, int i, void *arg, ZTK *ztk){
  zBox3DHeight((zBox3D*)obj) = ZTKDouble(ztk);
  return obj; }

static void _zBox3DCenterFPrint(FILE *fp, int i, void *obj){
  zVec3DFPrint( fp, zBox3DCenter((zBox3D*)obj) ); }
static void _zBox3DAxisXFPrint(FILE *fp, int i, void *obj){
  zVec3DFPrint( fp, zBox3DAxis((zBox3D*)obj,zX) ); }
static void _zBox3DAxisYFPrint(FILE *fp, int i, void *obj){
  zVec3DFPrint( fp, zBox3DAxis((zBox3D*)obj,zY) ); }
static void _zBox3DAxisZFPrint(FILE *fp, int i, void *obj){
  zVec3DFPrint( fp, zBox3DAxis((zBox3D*)obj,zZ) ); }
static void _zBox3DDepthFPrint(FILE *fp, int i, void *obj){
  fprintf( fp, "%.10g\n", zBox3DDepth((zBox3D*)obj) ); }
static void _zBox3DWidthFPrint(FILE *fp, int i, void *obj){
  fprintf( fp, "%.10g\n", zBox3DWidth((zBox3D*)obj) ); }
static void _zBox3DHeightFPrint(FILE *fp, int i, void *obj){
  fprintf( fp, "%.10g\n", zBox3DHeight((zBox3D*)obj) ); }

static ZTKPrp __ztk_prp_shape_box[] = {
  { "center", 1, _zBox3DCenterFromZTK, _zBox3DCenterFPrint },
  { "ax", 1, _zBox3DAxisXFromZTK, _zBox3DAxisXFPrint },
  { "ay", 1, _zBox3DAxisYFromZTK, _zBox3DAxisYFPrint },
  { "az", 1, _zBox3DAxisZFromZTK, _zBox3DAxisZFPrint },
  { "depth", 1, _zBox3DDepthFromZTK, _zBox3DDepthFPrint },
  { "width", 1, _zBox3DWidthFromZTK, _zBox3DWidthFPrint },
  { "height", 1, _zBox3DHeightFromZTK, _zBox3DHeightFPrint },
};

/* register a definition of tag-and-keys for a 3D box to a ZTK format processor. */
bool zBox3DDefRegZTK(ZTK *ztk, char *tag)
{
  return ZTKDefRegPrp( ztk, tag, __ztk_prp_shape_box );
}

/* read a 3D box from a ZTK format processor. */
zBox3D *zBox3DFromZTK(zBox3D *box, ZTK *ztk)
{
  zBox3DInit( box );
  return ZTKEncodeKey( box, NULL, ztk, __ztk_prp_shape_box );
}

/* scan a 3D box from a file. */
bool _zBox3DFScan(FILE *fp, void *instance, char *buf, bool *success)
{
  zVec3D a;

  if( strcmp( buf, "center" ) == 0 )
    zVec3DFScan( fp, zBox3DCenter( (zBox3D *)instance ) );
  else if( strcmp( buf, "ax" ) == 0 ){
    zVec3DFScan( fp, &a );
    zVec3DNormalize( &a, zBox3DAxis( (zBox3D *)instance, zX ) );
  } else if( strcmp( buf, "ay" ) == 0 ){
    zVec3DFScan( fp, &a );
    zVec3DNormalize( &a, zBox3DAxis( (zBox3D *)instance, zY ) );
  } else if( strcmp( buf, "az" ) == 0 ){
    zVec3DFScan( fp, &a );
    zVec3DNormalize( &a, zBox3DAxis( (zBox3D *)instance, zZ ) );
  } else if( strcmp( buf, "depth" ) == 0 )
    zBox3DSetDepth( (zBox3D *)instance, zFDouble( fp ) );
  else if( strcmp( buf, "width" ) == 0 )
    zBox3DSetWidth( (zBox3D *)instance, zFDouble( fp ) );
  else if( strcmp( buf, "height" ) == 0 )
    zBox3DSetHeight( (zBox3D *)instance, zFDouble( fp ) );
  else
    return false;
  return true;
}

/* scan a 3D box from a file. */
zBox3D *zBox3DFScan(FILE *fp, zBox3D *box)
{
  zBox3DInit( box );
  zFieldFScan( fp, _zBox3DFScan, box );
  return box;
}

/* print out a 3D box to a file. */
void zBox3DFPrint(FILE *fp, zBox3D *box)
{
  ZTKPrpKeyFPrint( fp, box, __ztk_prp_shape_box );
}

/* print a 3D box out to a file in a format to be plotted. */
void zBox3DDataFPrint(FILE *fp, zBox3D *box)
{
  zVec3D v[8];
  register int i;

  for( i=0; i<8; i++ )
    zBox3DVert( box, i, &v[i] );
  zVec3DDataNLFPrint( fp, &v[0] );
  zVec3DDataNLFPrint( fp, &v[1] );
  zVec3DDataNLFPrint( fp, &v[2] );
  zVec3DDataNLFPrint( fp, &v[3] );
  zVec3DDataNLFPrint( fp, &v[0] );
  zVec3DDataNLFPrint( fp, &v[4] );
  zVec3DDataNLFPrint( fp, &v[5] );
  zVec3DDataNLFPrint( fp, &v[6] );
  zVec3DDataNLFPrint( fp, &v[7] );
  zVec3DDataNLFPrint( fp, &v[4] );
  fprintf( fp, "\n" );
  zVec3DDataNLFPrint( fp, &v[1] );
  zVec3DDataNLFPrint( fp, &v[5] );
  fprintf( fp, "\n" );
  zVec3DDataNLFPrint( fp, &v[2] );
  zVec3DDataNLFPrint( fp, &v[6] );
  fprintf( fp, "\n" );
  zVec3DDataNLFPrint( fp, &v[3] );
  zVec3DDataNLFPrint( fp, &v[7] );
  fprintf( fp, "\n\n" );
}

/* methods for abstraction */

static void *_zShape3DInitBox(void* shape){
  return zBox3DInit( shape ); }
static void *_zShape3DAllocBox(void){
  return zBox3DAlloc(); }
static void *_zShape3DCloneBox(void *src){
  zBox3D *cln;
  return ( cln = zBox3DAlloc() ) ? zBox3DCopy( src, cln ) : NULL; }
static void *_zShape3DMirrorBox(void *src, zAxis axis){
  zBox3D *mrr;
  return ( mrr = zBox3DAlloc() ) ? zBox3DMirror( src, mrr, axis ) : NULL; }
static void _zShape3DDestroyBox(void *shape){}
static void *_zShape3DXformBox(void *src, zFrame3D *f, void *dest){
  return zBox3DXform( src, f, dest ); }
static void *_zShape3DXformInvBox(void *src, zFrame3D *f, void *dest){
  return zBox3DXformInv( src, f, dest ); }
static double _zShape3DClosestBox(void *shape, zVec3D *p, zVec3D *cp){
  return zBox3DClosest( shape, p, cp ); }
static double _zShape3DPointDistBox(void *shape, zVec3D *p){
  return zBox3DPointDist( shape, p ); }
static bool _zShape3DPointIsInsideBox(void *shape, zVec3D *p, bool rim){
  return zBox3DPointIsInside( shape, p, rim ); }
static double _zShape3DVolumeBox(void *shape){
  return zBox3DVolume( shape ); }
static zVec3D *_zShape3DBarycenterBox(void *shape, zVec3D *c){
  zVec3DCopy( zBox3DCenter((zBox3D*)shape), c ); return c; }
static zMat3D *_zShape3DInertiaBox(void *shape, zMat3D *i){
  return zBox3DInertia( shape, i ); }
static void _zShape3DBaryInertiaBox(void *shape, zVec3D *c, zMat3D *i){
  zVec3DCopy( zBox3DCenter((zBox3D*)shape), c );
  zBox3DInertia( shape, i ); }
static zPH3D *_zShape3DToPHBox(void *shape, zPH3D *ph){
  return zBox3DToPH( shape, ph ); }
static void *_zShape3DParseZTKBox(void *shape, ZTK *ztk){
  return zBox3DFromZTK( shape, ztk ); }
static void *_zShape3DFScanBox(FILE *fp, void *shape){
  return zBox3DFScan( fp, shape ); }
static void _zShape3DFPrintBox(FILE *fp, void *shape){
  return zBox3DFPrint( fp, shape ); }

zShape3DCom zeo_shape_box3d_com = {
  "box",
  _zShape3DInitBox,
  _zShape3DAllocBox,
  _zShape3DCloneBox,
  _zShape3DMirrorBox,
  _zShape3DDestroyBox,
  _zShape3DXformBox,
  _zShape3DXformInvBox,
  _zShape3DClosestBox,
  _zShape3DPointDistBox,
  _zShape3DPointIsInsideBox,
  _zShape3DVolumeBox,
  _zShape3DBarycenterBox,
  _zShape3DInertiaBox,
  _zShape3DBaryInertiaBox,
  _zShape3DToPHBox,
  _zShape3DParseZTKBox,
  _zShape3DFScanBox,
  _zShape3DFPrintBox,
};

/* create a 3D shape as a box. */
zShape3D *zShape3DCreateBox(zShape3D *shape, zVec3D *c, zVec3D *ax, zVec3D *ay, zVec3D *az, double d, double w, double h)
{
  zShape3DInit( shape );
  if( !( shape->body = zBox3DAlloc() ) ) return NULL;
  zBox3DCreate( zShape3DBox(shape), c, ax, ay, az, d, w, h );
  shape->com = &zeo_shape_box3d_com;
  return shape;
}

/* create a 3D shape as an axis-aligned box. */
zShape3D *zShape3DCreateBoxAlign(zShape3D *shape, zVec3D *c, double d, double w, double h)
{
  zShape3DInit( shape );
  if( !( shape->body = zBox3DAlloc() ) ) return NULL;
  zBox3DCreateAlign( zShape3DBox(shape), c, d, w, h );
  shape->com = &zeo_shape_box3d_com;
  return shape;
}