// Voro++, a cell-based Voronoi library
// By Chris H. Rycroft and the Rycroft Group

/** \file container_2d.cc
 * \brief Function implementations for the container_2d and related classes. */

#include "container_2d.hh"
#include "iter_2d.hh"

namespace voro {

/** The class constructor sets up the geometry of container, initializing the
 * minimum and maximum coordinates in each direction, and setting whether each
 * direction is periodic or not. It divides the container into a rectangular
 * grid of blocks, and allocates memory for each of these for storing particle
 * positions and IDs.
 * \param[in] (ax_,bx_) the minimum and maximum x coordinates.
 * \param[in] (ay_,by_) the minimum and maximum y coordinates.
 * \param[in] (nx_,ny_) the number of grid blocks in each of the three
 *                      coordinate directions.
 * \param[in] (x_prd_,y_prd_) flags setting whether the container is
 *                      periodic in each coordinate direction.
 * \param[in] init_mem the initial memory allocation for each block.
 * \param[in] ps_ the number of floating point entries to store for each
 *                particle. */
container_base_2d::container_base_2d(double ax_,double bx_,double ay_,double by_,
        int nx_,int ny_,bool x_prd_,bool y_prd_,int init_mem,int ps_)
    : voro_base_2d(nx_,ny_,(bx_-ax_)/nx_,(by_-ay_)/ny_),
    ax(ax_), bx(bx_), ay(ay_), by(by_), x_prd(x_prd_), y_prd(y_prd_),
    id(new int*[nxy]), p(new double*[nxy]), co(new int[nxy]), mem(new int[nxy]), ps(ps_) {
    int l;
    for(l=0;l<nxy;l++) co[l]=0;
    for(l=0;l<nxy;l++) mem[l]=init_mem;
    for(l=0;l<nxy;l++) id[l]=new int[init_mem];
    for(l=0;l<nxy;l++) p[l]=new double[ps*init_mem];
}

/** The container destructor frees the dynamically allocated memory. */
container_base_2d::~container_base_2d() {
    int l;
    for(l=nxy-1;l>=0;l--) delete [] p[l];
    for(l=nxy-1;l>=0;l--) delete [] id[l];
    delete [] id;
    delete [] p;
    delete [] co;
    delete [] mem;
}

/** The class constructor sets up the geometry of container.
 * \param[in] (ax_,bx_) the minimum and maximum x coordinates.
 * \param[in] (ay_,by_) the minimum and maximum y coordinates.
 * \param[in] (nx_,ny_) the number of grid blocks in each of the three
 *                      coordinate directions.
 * \param[in] (x_prd_,y_prd_) flags setting whether the container is periodic
 *                            in each coordinate direction.
 * \param[in] init_mem the initial memory allocation for each block. */
container_2d::container_2d(double ax_,double bx_,double ay_,double by_,
    int nx_,int ny_,bool x_prd_,bool y_prd_,int init_mem, int number_thread)
    : container_base_2d(ax_,bx_,ay_,by_,nx_,ny_,x_prd_,y_prd_,init_mem,2),
    nt(number_thread),vc(new voro_compute_2d<container_2d>*[nt]),
    overflow_mem_numPt(64), overflowPtCt(0), ijk_m_id_overflow(new int[3*overflow_mem_numPt]),
    p_overflow(new double[2*overflow_mem_numPt])
    {
        #pragma omp parallel num_threads(nt)
        {
        vc[t_num()]= new voro_compute_2d<container_2d>(*this,x_prd_?2*nx_+1:nx_,y_prd_?2*ny_+1:ny_);
        }
    }

container_2d::~container_2d(){
    int l;
    for(l=nt-1;l>=0;l--) delete vc[l];
    delete [] vc;
    //delete overflow arrays
    delete [] ijk_m_id_overflow;
    delete [] p_overflow;
}

void container_2d::change_number_thread(int number_thread){
    int l;
    for(l=nt-1;l>=0;l--) delete vc[l];
    delete [] vc;

    nt=number_thread;
    vc=new voro_compute_2d<container_2d>*[nt];
    #pragma omp parallel num_threads(nt)
    {
    vc[t_num()]= new voro_compute_2d<container_2d>(*this,x_prd?2*nx+1:nx,y_prd?2*ny+1:ny);
    }
}

/** The class constructor sets up the geometry of container.
 * \param[in] (ax_,bx_) the minimum and maximum x coordinates.
 * \param[in] (ay_,by_) the minimum and maximum y coordinates.
 * \param[in] (nx_,ny_) the number of grid blocks in each of the three
 *                      coordinate directions.
 * \param[in] (x_prd_,y_prd_) flags setting whether the container is periodic
 *                            in each coordinate direction.
 * \param[in] init_mem the initial memory allocation for each block. */
container_poly_2d::container_poly_2d(double ax_,double bx_,double ay_,double by_,
    int nx_,int ny_,bool x_prd_,bool y_prd_,int init_mem, int number_thread)
    : container_base_2d(ax_,bx_,ay_,by_,nx_,ny_,x_prd_,y_prd_,init_mem,3),
    nt(number_thread), vc(new voro_compute_2d<container_poly_2d>*[nt]),
    overflow_mem_numPt(64), overflowPtCt(0), ijk_m_id_overflow(new int[3*overflow_mem_numPt]),
    p_overflow(new double[3*overflow_mem_numPt])
    {
        max_r=new double[nt];
        #pragma omp parallel num_threads(nt)
        {
            vc[t_num()]=new voro_compute_2d<container_poly_2d>(*this,x_prd_?2*nx_+1:nx_,y_prd_?2*ny_+1:ny_);
            max_r[t_num()]=0.0;
        }
        ppr=p;
    }

container_poly_2d::~container_poly_2d(){
    int l;
    for(l=nt-1;l>=0;l--) delete vc[l];
    delete [] vc;
    delete [] max_r;
    //delete overflow arrays
    delete [] ijk_m_id_overflow;
    delete [] p_overflow;
}

void container_poly_2d::change_number_thread(int number_thread){
    int l;
    for(l=nt-1;l>=0;l--) delete vc[l];
    delete [] vc;
    delete [] max_r;

    nt=number_thread;
    vc=new voro_compute_2d<container_poly_2d>*[nt];
    max_r=new double[nt];
    #pragma omp parallel num_threads(nt)
    {
        vc[t_num()]=new voro_compute_2d<container_poly_2d>(*this,x_prd?2*nx+1:nx,y_prd?2*ny+1:ny);
        max_r[t_num()]=0.0;
    }
}

/** Put a particle into the correct region of the container.
 * \param[in] n the numerical ID of the inserted particle.
 * \param[in] (x,y) the position vector of the inserted particle. */
void container_2d::put(int n,double x,double y) {
    int ij;
    if(put_locate_block(ij,x,y)) {
        id[ij][co[ij]]=n;
        double *pp=p[ij]+2*co[ij]++;
        *(pp++)=x;*pp=y;
    }
}

/** Put a particle into the correct region of the container.
 * \param[in] i the numerical ID of the inserted particle.
 * \param[in] (x,y) the position vector of the inserted particle. */
void container_2d::put_parallel(int i,double x,double y) {
    int ij;
    //find block ijk that point (x,y,z) is in
    if(put_remap(ij,x,y)){
        int m;
        #pragma omp atomic capture
        m=co[ij]++;   //co[ij]: number of points in block ij

        //if m<init memory slot (mem[ijk]=init_mem in constructor), then add
        //pt and id info in id, p directly
        if(m<mem[ij]){
            id[ij][m]=i;
            double *pp=p[ij]+2*m;
            *(pp++)=x; *pp=y;
        }
        else{
            //in critical block: if(m>=mem[ij]), add pt info in overflow arrays:
            //ijk_m_id_overflow, p_overflow, and reconcile later
            #pragma omp critical
            {
                //opti: index of overflow point
                //overflowPtCt: number of overflow points
                int opti=overflowPtCt++;

                //if overflow arrays memory slots not enough, add new length
                if(overflowPtCt>overflow_mem_numPt){
                    int old_overflow_mem_numPt=overflow_mem_numPt;
                    overflow_mem_numPt=overflow_mem_numPt*2;
                    int *new_ijk_m_id_overflow=new int[3*overflow_mem_numPt];
                    double *new_p_overflow=new double [2*overflow_mem_numPt];
                    for(int ii=0;ii<old_overflow_mem_numPt;ii++){
                        int ii3=3*ii;
                        new_ijk_m_id_overflow[ii3]=ijk_m_id_overflow[ii3];
                        new_ijk_m_id_overflow[ii3+1]=ijk_m_id_overflow[ii3+1];
                        new_ijk_m_id_overflow[ii3+2]=ijk_m_id_overflow[ii3+2];

                        int ii2=2*ii;
                        new_p_overflow[ii2]=p_overflow[ii2];
                        new_p_overflow[ii2+1]=p_overflow[ii2+1];
                    }

                    delete [] ijk_m_id_overflow;
                    delete [] p_overflow;

                    ijk_m_id_overflow=new_ijk_m_id_overflow;
                    p_overflow=new_p_overflow;
                }

                //add in overflow info in overflow arrays
                int opti3=3*opti;
                ijk_m_id_overflow[opti3]=ij;
                ijk_m_id_overflow[opti3+1]=m;
                ijk_m_id_overflow[opti3+2]=i;

                int opti2=2*opti;
                p_overflow[opti2]=x;
                p_overflow[opti2+1]=y;
            }
        }
    }
}

//con.put with a input point array, parallel put
void container_2d::put_parallel(double *pt_list, int num_pt, int num_thread){
    #pragma omp parallel for num_threads(num_thread)
    for(int i=0; i<num_pt; i++){ //id:i
        double x=pt_list[2*i];
        double y=pt_list[2*i+1];
        put_parallel(i,x,y);
    }
}

void container_2d::put_reconcile_overflow(){
    //reconcile overflow
    //loop through points in overflow arrays,
    //if (m>=mem[ij]): int nmem (new memory length) = 2*mem[ij];
    //                  while(m>=nmem) {nmem=2*nmem;}
    // then construct new arrays of id, p with size nmem, and add in overflow pt info
    for(int i=0;i<overflowPtCt;i++){
        int i3=3*i;
        int ij=ijk_m_id_overflow[i3];
        int m=ijk_m_id_overflow[i3+1];
        int idd=ijk_m_id_overflow[i3+2];

        int i2=2*i;
        double x=p_overflow[i2];
        double y=p_overflow[i2+1];

        if(m>=mem[ij]){
            int nmem=2*mem[ij];
            while(m>=nmem){nmem=2*nmem;}
            int l;
            //the following are the same as add_particle_memory(ij)
            // Carry out a check on the memory allocation size, and
            // print a status message if requested
            if(nmem>max_particle_memory)
                    voro_fatal_error("Absolute maximum memory allocation exceeded",VOROPP_MEMORY_ERROR);
    #if VOROPP_VERBOSE >=3
            fprintf(stderr,"Particle memory in region %d scaled up to %d\n",i,nmem);
    #endif

            // Allocate new memory and copy in the contents of the old arrays
            int *idp=new int[nmem];
            for(l=0;l<mem[ij];l++) {
                idp[l]=id[ij][l];
            }

            double *pp=new double[ps*nmem];
            for(l=0;l<ps*mem[ij];l++) {
                pp[l]=p[ij][l];
            }
            // Update pointers and delete old arrays
            mem[ij]=nmem;
            delete [] id[ij];
            delete [] p[ij];
            id[ij]=idp;
            p[ij]=pp;
        }
        //after fixing memory issue above, now, add overflow pt information into the original p, id arrays
        id[ij][m]=idd;
        double *pp=p[ij]+2*m;
        *(pp++)=x; *pp=y;
    }
    //after reconciling, set overflowPtCt back to initial value
    overflowPtCt=0;
}

/** Put a particle into the correct region of the container.
 * \param[in] n the numerical ID of the inserted particle.
 * \param[in] (x,y) the position vector of the inserted particle.
 * \param[in] r the radius of the particle. */
void container_poly_2d::put(int n,double x,double y,double r) {
    int ij;
    if(put_locate_block(ij,x,y)) {
        id[ij][co[ij]]=n;
        double *pp=p[ij]+3*co[ij]++;
        *(pp++)=x;*(pp++)=y;*pp=r;
        if(max_radius<r) max_radius=r;
    }
}

/** Put a particle into the correct region of the container.
 * \param[in] i the numerical ID of the inserted particle.
 * \param[in] (x,y) the position vector of the inserted particle.
 * \param[in] r the radius of the particle. */
void container_poly_2d::put_parallel(int i,double x,double y,double r) {
    int ithread=t_num();
    int ij;
    //find block ijk that point (x,y,z) is in
    if(put_remap(ij,x,y)){
        int m;
        #pragma omp atomic capture
        m=co[ij]++;   //co[ij]: number of points in block ij

        //if m<init memory slot (mem[ijk]=init_mem in constructor), then add
        //pt and id info in id, p directly
        if(m<mem[ij]){
            id[ij][m]=i;
            double *pp=p[ij]+3*m;
            *(pp++)=x;*(pp++)=y;*pp=r;
            //max_r
            if(max_r[ithread]<r) {max_r[ithread]=r;}
        }
        else{
            //in critical block: if(m>=mem[ij]), add pt info in overflow arrays:
            //ijk_m_id_overflow, p_overflow, and reconcile later
            #pragma omp critical
            {
                //opti: index of overflow point
                //overflowPtCt: number of overflow points
                int opti=overflowPtCt++;

                //if overflow arrays memory slots not enough, add new length
                if(overflowPtCt>overflow_mem_numPt){
                    int old_overflow_mem_numPt=overflow_mem_numPt;
                    overflow_mem_numPt=overflow_mem_numPt*2;
                    int *new_ijk_m_id_overflow=new int[3*overflow_mem_numPt];
                    double *new_p_overflow=new double [3*overflow_mem_numPt];
                    for(int ii=0;ii<old_overflow_mem_numPt;ii++){
                        int ii3=3*ii;
                        new_ijk_m_id_overflow[ii3]=ijk_m_id_overflow[ii3];
                        new_ijk_m_id_overflow[ii3+1]=ijk_m_id_overflow[ii3+1];
                        new_ijk_m_id_overflow[ii3+2]=ijk_m_id_overflow[ii3+2];

                        new_p_overflow[ii3]=p_overflow[ii3];
                        new_p_overflow[ii3+1]=p_overflow[ii3+1];
                        new_p_overflow[ii3+2]=p_overflow[ii3+2];
                    }

                    delete [] ijk_m_id_overflow;
                    delete [] p_overflow;

                    ijk_m_id_overflow=new_ijk_m_id_overflow;
                    p_overflow=new_p_overflow;
                }
                //add in overflow info in overflow arrays
                int opti3=3*opti;
                ijk_m_id_overflow[opti3]=ij;
                ijk_m_id_overflow[opti3+1]=m;
                ijk_m_id_overflow[opti3+2]=i;

                p_overflow[opti3]=x;
                p_overflow[opti3+1]=y;
                p_overflow[opti3+2]=r;

                //max_r
                if(max_r[ithread]<r) {max_r[ithread]=r;}
            }
        }
    }
}

//con.put with a input point array, parallel put
void container_poly_2d::put_parallel(double *pt_r_list, int num_pt, int num_thread){
    #pragma omp parallel for num_threads(num_thread)
    for(int i=0; i<num_pt; i++){ //id:i
        double x=pt_r_list[3*i];
        double y=pt_r_list[3*i+1];
        double r=pt_r_list[3*i+2];
        put_parallel(i,x,y,r);
    }
}

void container_poly_2d::put_reconcile_overflow(){
    //update max_radius
    //and reset max_r[i]
    for(int i=0;i<nt;i++){
        if(max_radius<max_r[i]){max_radius=max_r[i];}
        max_r[i]=0.0;
    }

    //reconcile overflow
    //loop through points in overflow arrays,
    //if (m>=mem[ij]): int nmem (new memory length) = 2*mem[ij];
    //                  while(m>=nmem) {nmem=2*nmem;}
    // then construct new arrays of id, p with size nmem, and add in overflow pt info
    for(int i=0;i<overflowPtCt;i++){
        int i3=3*i;
        int ij=ijk_m_id_overflow[i3];
        int m=ijk_m_id_overflow[i3+1];
        int idd=ijk_m_id_overflow[i3+2];

        double x=p_overflow[i3];
        double y=p_overflow[i3+1];
        double r=p_overflow[i3+2];

        if(m>=mem[ij]){
            int nmem=2*mem[ij];
            while(m>=nmem){nmem=2*nmem;}
            int l;
            //the following are the same as add_particle_memory(ij)
            // Carry out a check on the memory allocation size, and
            // print a status message if requested
            if(nmem>max_particle_memory)
                    voro_fatal_error("Absolute maximum memory allocation exceeded",VOROPP_MEMORY_ERROR);
    #if VOROPP_VERBOSE >=3
            fprintf(stderr,"Particle memory in region %d scaled up to %d\n",i,nmem);
    #endif

            // Allocate new memory and copy in the contents of the old arrays
            int *idp=new int[nmem];
            for(l=0;l<mem[ij];l++) {
                idp[l]=id[ij][l];
            }

            double *pp=new double[ps*nmem];
            for(l=0;l<ps*mem[ij];l++) {
                pp[l]=p[ij][l];
            }
            // Update pointers and delete old arrays
            mem[ij]=nmem;
            delete [] id[ij];
            delete [] p[ij];
            id[ij]=idp;
            p[ij]=pp;
        }
        //after fixing memory issue above, now, add overflow pt information into the original p, id arrays
        id[ij][m]=idd;
        double *pp=p[ij]+3*m;
        *(pp++)=x; *(pp++)=y; *pp=r;
    }
    //after reconciling, set overflowPtCt back to initial value
    overflowPtCt=0;
}

/** Put a particle into the correct region of the container, also recording
 * into which region it was stored.
 * \param[in] vo the ordering class in which to record the region.
 * \param[in] n the numerical ID of the inserted particle.
 * \param[in] (x,y) the position vector of the inserted particle. */
void container_2d::put(particle_order &vo,int n,double x,double y) {
    int ij;
    if(put_locate_block(ij,x,y)) {
        id[ij][co[ij]]=n;
        vo.add(ij,co[ij]);
        double *pp=p[ij]+2*co[ij]++;
        *(pp++)=x;*pp=y;
    }
}

/** Put a particle into the correct region of the container, also recording
 * into which region it was stored.
 * \param[in] vo the ordering class in which to record the region.
 * \param[in] n the numerical ID of the inserted particle.
 * \param[in] (x,y) the position vector of the inserted particle.
 * \param[in] r the radius of the particle. */
void container_poly_2d::put(particle_order &vo,int n,double x,double y,double r) {
    int ij;
    if(put_locate_block(ij,x,y)) {
        id[ij][co[ij]]=n;
        vo.add(ij,co[ij]);
        double *pp=p[ij]+3*co[ij]++;
        *(pp++)=x;*(pp++)=y;*pp=r;
        if(max_radius<r) max_radius=r;
    }
}

/** This routine takes a particle position vector, tries to remap it into the
 * primary domain. If successful, it computes the region into which it can be
 * stored and checks that there is enough memory within this region to store
 * it.
 * \param[out] ij the region index.
 * \param[in,out] (x,y) the particle position, remapped into the primary
 *                      domain if necessary.
 * \return True if the particle can be successfully placed into the container,
 * false otherwise. */
inline bool container_base_2d::put_locate_block(int &ij,double &x,double &y) {
    if(put_remap(ij,x,y)) {
        if(co[ij]==mem[ij]) add_particle_memory(ij);
        return true;
    }
#if VOROPP_REPORT_OUT_OF_BOUNDS ==1
    fprintf(stderr,"Out of bounds: (x,y)=(%g,%g)\n",x,y);
#endif
    return false;
}

/** Takes a particle position vector and computes the region index into which
 * it should be stored. If the container is periodic, then the routine also
 * maps the particle position to ensure it is in the primary domain. If the
 * container is not periodic, the routine bails out.
 * \param[out] ij the region index.
 * \param[in,out] (x,y) the particle position, remapped into the primary domain
 *                      if necessary.
 * \return True if the particle can be successfully placed into the container,
 * false otherwise. */
inline bool container_base_2d::put_remap(int &ij,double &x,double &y) {
    int l;

    ij=step_int((x-ax)*xsp);
    if(x_prd) {l=step_mod(ij,nx);x+=boxx*(l-ij);ij=l;}
    else if(ij<0||ij>=nx) return false;

    int j=step_int((y-ay)*ysp);
    if(y_prd) {l=step_mod(j,ny);y+=boxy*(l-j);j=l;}
    else if(j<0||j>=ny) return false;

    ij+=nx*j;
    return true;
}

/** Takes a position vector and attempts to remap it into the primary domain.
 * \param[out] (ai,aj) the periodic image displacement that the vector is in,
 *                     with (0,0,0) corresponding to the primary domain.
 * \param[out] (ci,cj) the index of the block that the position vector is
 *                     within, once it has been remapped.
 * \param[in,out] (x,y) the position vector to consider, which is remapped into
 *                      the primary domain during the routine.
 * \param[out] ij the block index that the vector is within.
 * \return True if the particle is within the container or can be remapped into
 * it, false if it lies outside of the container bounds. */
inline bool container_base_2d::remap(int &ai,int &aj,int &ci,int &cj,double &x,double &y,int &ij) {
    ci=step_int((x-ax)*xsp);
    if(ci<0||ci>=nx) {
        if(x_prd) {ai=step_div(ci,nx);x-=ai*(bx-ax);ci-=ai*nx;}
        else return false;
    } else ai=0;

    cj=step_int((y-ay)*ysp);
    if(cj<0||cj>=ny) {
        if(y_prd) {aj=step_div(cj,ny);y-=aj*(by-ay);cj-=aj*ny;}
        else return false;
    } else aj=0;

    ij=ci+nx*cj;
    return true;
}

/** Takes a vector and finds the particle whose Voronoi cell contains that
 * vector. This is equivalent to finding the particle which is nearest to the
 * vector. Additional wall classes are not considered by this routine.
 * \param[in] (x,y) the vector to test.
 * \param[out] (rx,ry) the position of the particle whose Voronoi cell contains
 *                     the vector. If the container is periodic, this may point
 *                     to a particle in a periodic image of the primary domain.
 * \param[out] pid the ID of the particle.
 * \return True if a particle was found. If the container has no particles,
 * then the search will not find a Voronoi cell and false is returned. */
bool container_2d::find_voronoi_cell(double x,double y,double &rx,double &ry,int &pid) {
    int ai,aj,ci,cj,ij;
    particle_record_2d w;
    double mrs;

    // If the given vector lies outside the domain, but the container is
    // periodic, then remap it back into the domain
    if(!remap(ai,aj,ci,cj,x,y,ij)) return false;
    const int tn=t_num();
    vc[tn]->find_voronoi_cell(x,y,ci,cj,ij,w,mrs);

    if(w.ij!=-1) {

        // Assemble the position vector of the particle to be returned,
        // applying a periodic remapping if necessary
        if(x_prd) {ci+=w.di;if(ci<0||ci>=nx) ai+=step_div(ci,nx);}
        if(y_prd) {cj+=w.dj;if(cj<0||cj>=ny) aj+=step_div(cj,ny);}
        rx=p[w.ij][2*w.l]+ai*(bx-ax);
        ry=p[w.ij][2*w.l+1]+aj*(by-ay);
        pid=id[w.ij][w.l];
        return true;
    }

    // If no particle is found then just return false
    return false;
}

/** Takes a vector and finds the particle whose Voronoi cell contains that
 * vector. Additional wall classes are not considered by this routine.
 * \param[in] (x,y) the vector to test.
 * \param[out] (rx,ry) the position of the particle whose Voronoi cell contains
 *                     the vector. If the container is periodic, this may point
 *                     to a particle in a periodic image of the primary domain.
 * \param[out] pid the ID of the particle.
 * \return True if a particle was found. If the container has no particles,
 * then the search will not find a Voronoi cell and false is returned. */
bool container_poly_2d::find_voronoi_cell(double x,double y,double &rx,double &ry,int &pid) {
    int ai,aj,ci,cj,ij;
    particle_record_2d w;
    double mrs;

    // If the given vector lies outside the domain, but the container is
    // periodic, then remap it back into the domain
    if(!remap(ai,aj,ci,cj,x,y,ij)) return false;
    const int tn=t_num();
    vc[tn]->find_voronoi_cell(x,y,ci,cj,ij,w,mrs);

    if(w.ij!=-1) {

        // Assemble the position vector of the particle to be returned,
        // applying a periodic remapping if necessary
        if(x_prd) {ci+=w.di;if(ci<0||ci>=nx) ai+=step_div(ci,nx);}
        if(y_prd) {cj+=w.dj;if(cj<0||cj>=ny) aj+=step_div(cj,ny);}
        rx=p[w.ij][3*w.l]+ai*(bx-ax);
        ry=p[w.ij][3*w.l+1]+aj*(by-ay);
        pid=id[w.ij][w.l];
        return true;
    }

    // If no particle is found then just return false
    return false;
}

/** Increase memory for a particular region.
 * \param[in] i the index of the region to reallocate. */
void container_base_2d::add_particle_memory(int i) {
    int l,nmem=mem[i]<<1;

    // Carry out a check on the memory allocation size, and print a status
    // message if requested
    if(nmem>max_particle_memory)
        voro_fatal_error("Absolute maximum memory allocation exceeded",VOROPP_MEMORY_ERROR);
#if VOROPP_VERBOSE >=3
    fprintf(stderr,"Particle memory in region %d scaled up to %d\n",i,nmem);
#endif

    // Allocate new memory and copy in the contents of the old arrays
    int *idp=new int[nmem];
    for(l=0;l<co[i];l++) idp[l]=id[i][l];
    double *pp=new double[ps*nmem];
    for(l=0;l<ps*co[i];l++) pp[l]=p[i][l];

    // Update pointers and delete old arrays
    mem[i]=nmem;
    delete [] id[i];id[i]=idp;
    delete [] p[i];p[i]=pp;
}

/** Import a list of particles from an open file stream into the container.
 * Entries of four numbers (Particle ID, x position, y position, z position)
 * are searched for. If the file cannot be successfully read, then the routine
 * causes a fatal error.
 * \param[in] fp the file handle to read from. */
void container_2d::import(FILE *fp) {
    int i,j;
    double x,y;
    while((j=fscanf(fp,"%d %lg %lg",&i,&x,&y))==3) {put(i,x,y);}
     put_reconcile_overflow();
    if(j!=EOF) voro_fatal_error("File import error",VOROPP_FILE_ERROR);
}

/** Import a list of particles from an open file stream, also storing the order
 * of that the particles are read. Entries of four numbers (Particle ID, x
 * position, y position, z position) are searched for. If the file cannot be
 * successfully read, then the routine causes a fatal error.
 * \param[in,out] vo a reference to an ordering class to use.
 * \param[in] fp the file handle to read from. */
void container_2d::import(particle_order &vo,FILE *fp) {
    int i,j;
    double x,y;
    while((j=fscanf(fp,"%d %lg %lg",&i,&x,&y))==3) put(vo,i,x,y);
    if(j!=EOF) voro_fatal_error("File import error",VOROPP_FILE_ERROR);
}

/** Import a list of particles from an open file stream into the container.
 * Entries of five numbers (Particle ID, x position, y position, z position,
 * radius) are searched for. If the file cannot be successfully read, then the
 * routine causes a fatal error.
 * \param[in] fp the file handle to read from. */
void container_poly_2d::import(FILE *fp) {
    int i,j;
    double x,y,r;
    while((j=fscanf(fp,"%d %lg %lg %lg",&i,&x,&y,&r))==4) {put(i,x,y,r);}
    put_reconcile_overflow();
    if(j!=EOF) voro_fatal_error("File import error",VOROPP_FILE_ERROR);
}

/** Import a list of particles from an open file stream, also storing the order
 * of that the particles are read. Entries of four numbers (Particle ID, x
 * position, y position, z position, radius) are searched for. If the file
 * cannot be successfully read, then the routine causes a fatal error.
 * \param[in,out] vo a reference to an ordering class to use.
 * \param[in] fp the file handle to read from. */
void container_poly_2d::import(particle_order &vo,FILE *fp) {
    int i,j;
    double x,y,r;
    while((j=fscanf(fp,"%d %lg %lg %lg",&i,&x,&y,&r))==4) put(vo,i,x,y,r);
    if(j!=EOF) voro_fatal_error("File import error",VOROPP_FILE_ERROR);
}

/** Outputs the a list of all the container regions along with the number of
 * particles stored within each. */
void container_base_2d::region_count() {
    int i,j,*cop=co;
    for(j=0;j<ny;j++) for(i=0;i<nx;i++)
        printf("Region (%d,%d): %d particles\n",i,j,*(cop++));
}

/** Clears a container of particles. */
void container_2d::clear() {
    for(int *cop=co;cop<co+nxy;cop++) *cop=0;
}

/** Clears a container of particles, also clearing resetting the maximum radius
 * to zero. */
void container_poly_2d::clear() {
    for(int *cop=co;cop<co+nxy;cop++) *cop=0;
    max_radius=0;
}

/** This function tests to see if a given vector lies within the container
 * bounds and any walls.
 * \param[in] (x,y) the position vector to be tested.
 * \return True if the point is inside the container, false if the point is
 * outside. */
bool container_base_2d::point_inside(double x,double y) {
    if(x<ax||x>bx||y<ay||y>by) return false;
    return point_inside_walls(x,y);
}

/** Draws an outline of the domain in Gnuplot format.
 * \param[in] fp the file handle to write to. */
void container_base_2d::draw_domain_gnuplot(FILE *fp) {
    fprintf(fp,"%g %g\n%g %g\n%g %g\n%g %g\n%g %g\n",ax,ay,bx,ay,bx,by,ax,by,ax,ay);
}

/** Draws an outline of the domain in POV-Ray format.
 * \param[in] fp the file handle to write to. */
void container_base_2d::draw_domain_pov(FILE *fp) {
    fprintf(fp,"cylinder{<%g,%g,0>,<%g,%g,0>,rr}\n"
               "cylinder{<%g,%g,0>,<%g,%g,0>,rr}\n"
               "cylinder{<%g,%g,0>,<%g,%g,0>,rr}\n"
               "cylinder{<%g,%g,0>,<%g,%g,0>,rr}\n"
               "sphere{<%g,%g,0>,rr}\nsphere{<%g,%g,0>,rr}\n"
               "sphere{<%g,%g,0>,rr}\nsphere{<%g,%g,0>,rr}\n",
               ax,ay,bx,ay,ax,by,bx,by,ax,ay,ax,by,
               bx,ay,bx,by,ax,ay,bx,ay,ax,by,bx,by);
}

/** Dumps particle IDs and positions to a file.
 * \param[in] fp a file handle to write to. */
void container_2d::draw_particles(FILE *fp) {
    for(iterator cli=begin();cli<end();cli++) {
        int ij=cli->ijk,q=cli->q;
        double *pp=p[ij]+2*q;
        fprintf(fp,"%d %g %g\n",id[ij][q],*pp,pp[1]);
    }
}

/** Dumps particle positions in POV-Ray format.
 * \param[in] fp a file handle to write to. */
void container_2d::draw_particles_pov(FILE *fp) {
    for(iterator cli=begin();cli<end();cli++) {
        int ij=cli->ijk,q=cli->q;
        double *pp=p[ij]+2*q;
        fprintf(fp,"// id %d\nsphere{<%g,%g,0>,s}\n",id[ij][q],*pp,pp[1]);
    }
}

/** Computes Voronoi cells and saves the output in Gnuplot format.
 * \param[in] fp a file handle to write to. */
void container_2d::draw_cells_gnuplot(FILE *fp) {
    voronoicell_2d c;
    for(iterator cli=begin();cli<end();cli++) if(compute_cell(c,cli)) {
        double *pp=p[cli->ijk]+2*cli->q;
        c.draw_gnuplot(*pp,pp[1],fp);
    }
}

/** Computes Voronoi cells and saves the output in POV-Ray format.
 * \param[in] fp a file handle to write to. */
void container_2d::draw_cells_pov(FILE *fp) {
    voronoicell_2d c;
    for(iterator cli=begin();cli<end();cli++) if(compute_cell(c,cli)) {
        int ij=cli->ijk,q=cli->q;
        double *pp=p[ij]+2*q;
        fprintf(fp,"// cell %d\n",id[ij][q]);
        c.draw_pov(*pp,pp[1],fp);
    }
}

/** Computes the Voronoi cells and saves customized information about them.
 * \param[in] format the custom output string to use.
 * \param[in] fp a file handle to write to. */
void container_2d::print_custom(const char *format,FILE *fp) {
    int ij,q;double *pp;
    if(voro_contains_neighbor(format)) {
        voronoicell_neighbor_2d c;
        for(iterator cli=begin();cli<end();cli++) if(compute_cell(c,cli)) {
            ij=cli->ijk;q=cli->q;
            pp=p[ij]+2*q;
            c.output_custom(format,id[ij][q],*pp,pp[1],default_radius,fp);
        }
    } else {
        voronoicell_2d c;
        for(iterator cli=begin();cli<end();cli++) if(compute_cell(c,cli)) {
            ij=cli->ijk;q=cli->q;
            pp=p[ij]+2*q;
            c.output_custom(format,id[ij][q],*pp,pp[1],default_radius,fp);
        }
    }
}

/** Computes all of the Voronoi cells in the container, but does nothing with
 * the output. It is useful for measuring the pure computation time of the
 * Voronoi algorithm, without any additional calculations such as volume
 * evaluation or cell output. */
void container_2d::compute_all_cells() {
    voronoicell_2d c;
    for(iterator cli=begin();cli<end();cli++) compute_cell(c,cli);
}

/** Calculates all of the Voronoi cells and sums their volumes. In most cases
 * without walls, the sum of the Voronoi cell volumes should equal the volume
 * of the container to numerical precision.
 * \return The sum of all of the computed Voronoi volumes. */
double container_2d::sum_cell_areas() {
    voronoicell_2d c;
    double area=0;
    for(iterator cli=begin();cli<end();cli++) if(compute_cell(c,cli)) area+=c.area();
    return area;
}

/** Dumps particle IDs and positions to a file.
 * \param[in] fp a file handle to write to. */
void container_poly_2d::draw_particles(FILE *fp) {
    for(iterator cli=begin();cli<end();cli++) {
        int ij=cli->ijk,q=cli->q;
        double *pp=p[ij]+3*q;
        fprintf(fp,"%d %g %g %g\n",id[ij][q],*pp,pp[1],pp[2]);
    }
}

/** Dumps particle positions in POV-Ray format.
 * \param[in] fp a file handle to write to. */
void container_poly_2d::draw_particles_pov(FILE *fp) {
    for(iterator cli=begin();cli<end();cli++) {
        int ij=cli->ijk,q=cli->q;
        double *pp=p[ij]+3*q;
        fprintf(fp,"// id %d\nsphere{<%g,%g,0>,%g}\n",id[ij][q],*pp,pp[1],pp[2]);
    }
}

/** Computes Voronoi cells and saves the output in Gnuplot format.
 * \param[in] fp a file handle to write to. */
void container_poly_2d::draw_cells_gnuplot(FILE *fp) {
    voronoicell_2d c;
    for(iterator cli=begin();cli<end();cli++) if(compute_cell(c,cli)) {
        double *pp=p[cli->ijk]+3*cli->q;
        c.draw_gnuplot(*pp,pp[1],fp);;
    }
}

/** Computes all Voronoi cells and saves the output in POV-Ray format.
 * \param[in] fp a file handle to write to. */
void container_poly_2d::draw_cells_pov(FILE *fp) {
    voronoicell_2d c;
    for(iterator cli=begin();cli<end();cli++) if(compute_cell(c,cli)) {
        int ij=cli->ijk,q=cli->q;
        double *pp=p[ij]+3*q;
        fprintf(fp,"// cell %d\n",id[ij][q]);
        c.draw_pov(*pp,pp[1],fp);
    }
}

/** Computes the Voronoi cells and saves customized information about
 * them.
 * \param[in] cli the iterator class to use.
 * \param[in] format the custom output string to use.
 * \param[in] fp a file handle to write to. */
void container_poly_2d::print_custom(const char *format,FILE *fp) {
    int ij,q;double *pp;
    if(voro_contains_neighbor(format)) {
        voronoicell_neighbor_2d c;
        for(iterator cli=begin();cli<end();cli++) if(compute_cell(c,cli)) {
            ij=cli->ijk;q=cli->q;
            pp=p[ij]+3*q;
            c.output_custom(format,id[ij][q],*pp,pp[1],pp[2],fp);
        }
    } else {
        voronoicell_2d c;
        for(iterator cli=begin();cli<end();cli++) if(compute_cell(c,cli)) {
            ij=cli->ijk;q=cli->q;
            pp=p[ij]+3*q;
            c.output_custom(format,id[ij][q],*pp,pp[1],pp[2],fp);
        }
    }
}

/** Computes all of the Voronoi cells in the container, but does nothing with
 * the output. It is useful for measuring the pure computation time of the
 * Voronoi algorithm, without any additional calculations such as volume
 * evaluation or cell output. */
void container_poly_2d::compute_all_cells() {
    voronoicell_2d c;
    for(iterator cli=begin();cli<end();cli++) compute_cell(c,cli);
}

/** Calculates all of the Voronoi cells and sums their volumes. In most cases
 * without walls, the sum of the Voronoi cell volumes should equal the volume
 * of the container to numerical precision.
 * \return The sum of all of the computed Voronoi volumes. */
double container_poly_2d::sum_cell_areas() {
    voronoicell_2d c;
    double area=0;
    for(iterator cli=begin();cli<end();cli++) if(compute_cell(c,cli)) area+=c.area();
    return area;
}

}
