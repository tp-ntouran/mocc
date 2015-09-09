#include "moc_sweeper.hpp"

#include <iomanip>
#include "error.hpp"

using std::endl;
using std::cout;

namespace mocc {
    
    MoCSweeper::MoCSweeper( const pugi::xml_node& input, 
            const CoreMesh& mesh ):
        TransportSweeper( mesh ),
        mesh_( mesh ),
        ang_quad_( input.child("ang_quad") ),
        rays_( input.child("rays"), ang_quad_, mesh ),
        xstr_( n_reg_, 1 ),
        flux_1g_( n_reg_, 1 ),
        qbar_( n_reg_, 1 ),
        bc_type_( mesh_.boundary() )
    {   
        // Make sure we have input from the XML
        if( input.empty() ) {
            Error("No input specified to initialize MoC sweeper.");
        }

        // Parse the number of inner iterations
        int int_in = input.attribute("n_inner").as_int(-1);
        if(int_in < 0) {
            Error("Invalid number of inner iterations specified (n_inner).");
        }
        n_inner_ = int_in;

        // Set up the array of volumes (surface area)
        int ireg = 0;
        for( auto &pin: mesh_ ) {
            const VecF& pin_vols = pin->vols();
            for( auto &v: pin_vols ) {
                vol_(ireg) = v;
                ireg++;
            }
        }

        // allocate space to store the boundary conditions
        boundary_.resize( ng_ );
        for( auto &group_rays: boundary_ ) {
            group_rays.resize( mesh_.nz() );
            for( auto &angle_rays: group_rays ) {
                // We actually allocate BCs for all 4 octants to make things a
                // little simpler.
                angle_rays.resize( ang_quad_.ndir_oct()*4 );
                int iang = 0;
                for( auto ang_it=ang_quad_.octant(1); 
                        ang_it!=ang_quad_.octant(5); ang_it++ ) {
                    angle_rays[iang].resize(rays_.n_rays(iang));
                    iang++;
                }
            }
        }
        boundary_out_.resize( mesh_.nz() );
        for( auto &angle_rays: boundary_out_ ) {
            // We actually allocate BCs for all 4 octants to make things a
            // little simpler.
            angle_rays.resize( ang_quad_.ndir_oct()*4 );
            int iang = 0;
            for( auto ang_it=ang_quad_.octant(1); 
                    ang_it!=ang_quad_.octant(5); ang_it++ ) {
                angle_rays[iang].resize(rays_.n_rays(iang));
                iang++;
            }
        }

        return;
    }

    void MoCSweeper::sweep( int group ) {
        // set up the xstr_ array
        for( auto &xsr: *xs_mesh_ ) {
            float_t xstr = xsr.xsmactr()[group];
            for( auto &ireg: xsr.reg() ) {
                xstr_(ireg) = xstr;
            }
        }

        flux_1g_ = flux_.col( group );

        // Perform inner iterations
        for( unsigned int inner=0; inner<n_inner_; inner++ ) {
            // update the self-scattering source
            source_->self_scatter( group, flux_1g_, qbar_ );
            // Perform the stock sweep unless we are on the last outer and have
            // a CoarseData object.
            if( inner == n_inner_-1 && coarse_data_ ) {
                this->sweep1g_current( group );
            } else {
                this->sweep1g( group );
            }
        }

        flux_.col( group ) = flux_1g_;

        return;
    }


    void MoCSweeper::update_boundary( int group ) {
        int iplane=0;
        for( auto &plane_bcs: boundary_[group] ) {
            for( unsigned int iang=0; iang<plane_bcs.size(); iang++ ) {
                int nx = rays_.nx(iang);
                int ny = rays_.ny(iang);

                // Determine based on the angle quadrant which surfaces are
                // upwind and need to be updated for the given angle
                Surface upwind[2];
                if(ang_quad_[iang].ox > 0.0) {
                    upwind[0] = Surface::WEST;
                } else {
                    upwind[0] = Surface::EAST;
                }
                if(ang_quad_[iang].oy > 0.0) {
                    upwind[1] = Surface::SOUTH;
                } else {
                    upwind[1] = Surface::NORTH;
                }

                // Treat the two surfaces that were determined above
                if( bc_type_[(int)upwind[0]] == Boundary::REFLECT ) {
                    int ang_ref = ang_quad_.reflect(iang, upwind[0]);
                    for( int ibc=0; ibc<ny; ibc++ ) {
                        plane_bcs[iang][ibc] =
                            boundary_out_[iplane][ang_ref][ibc];
                    }
                } else {
                    for( int ibc=0; ibc<ny; ibc++ ) {
                        plane_bcs[iang][ibc] = 0.0;
                    }
                }
                
                if( bc_type_[(int)upwind[1]] == Boundary::REFLECT ) {
                    int ang_ref = ang_quad_.reflect(iang, upwind[1]);
                    for( int ibc=ny; ibc<(nx+ny); ibc++ ) {
                        plane_bcs[iang][ibc] =
                            boundary_out_[iplane][ang_ref][ibc];
                    }
                } else {
                    for( int ibc=ny; ibc<(nx+ny); ibc++ ) {
                        plane_bcs[iang][ibc] = 0.0;
                    }
                }
            } // angle loop
            iplane++;
        } // plane loop
    }

    void MoCSweeper::initialize() {
        // There are better ways to do this, but for now, just start with 1.0
        flux_.fill(1.0);

        // Walk through the boundary conditions and initialize them the 1/4pi
        float_t val = 1.0/FPI;
        for( auto &group_rays: boundary_ ) {
            for( auto &plane_rays: group_rays ) {
                for( auto &angle_rays: plane_rays ) {
                    for( auto &ray: angle_rays ) {
                        ray = val;
                    }
                }
            }
        }
        return;
    }

    void MoCSweeper::homogenize( CoarseData &data ) const {
        
        return;
    }

    void MoCSweeper::get_pin_flux( int group, VecF& flux ) const {
        flux.resize( mesh_.n_pin() );
        for( auto &f: flux ) {
            f = 0.0;
        }

        int ireg = 0;
        int ipin = 0;
        for( auto &pin: mesh_ ) {
            Position pos = mesh_.pin_position(ipin);
            int i = mesh_.index_lex(pos);
            for( int ir=0; ir<pin->n_reg(); ir++) {
                flux[i] += flux_(ireg, group)*vol_(ireg);
                ireg++;
            }
            ipin++;
        }

        return;
    }

    void MoCSweeper::output( H5File& file ) const {
        // Get core dimensions from the mesh
        const int nx = mesh_.nx();
        const int ny = mesh_.ny();
        const int nz = mesh_.nz();
        VecI dims;
        dims.push_back(nz);
        dims.push_back(ny);
        dims.push_back(nx);
        
        // Make a group in the file to store the flux
        file.mkdir("/flux");
        
        // Provide energy group upper bounds
        file.write("/eubounds", xs_mesh_->eubounds(), VecI(1, ng_));
        file.write("/ng", ng_);
        
        for( unsigned int ig=0; ig<ng_; ig++ ) {
            VecF flux;
            this->get_pin_flux(ig, flux);
        
        
            std::stringstream setname;
            setname << "/flux/" << std::setfill('0') << std::setw(3) << ig+1;
        
            file.write(setname.str(), flux, dims);
        }

        return;
    }

#include "moc_kernels.hpp.inc"
}
