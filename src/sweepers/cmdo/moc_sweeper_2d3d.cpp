#include "moc_sweeper_2d3d.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <valarray>

#include "correction_worker.hpp"
#include "files.hpp"

using std::cout;
using std::cin;
using std::endl;

namespace mocc {
    MoCSweeper_2D3D::MoCSweeper_2D3D( const pugi::xml_node &input,
            const CoreMesh &mesh ):
        MoCSweeper( input, mesh ),
        corrections_( nullptr )
    {
        LogFile << "Constructing a 2D3D MoC sweeper" << std::endl;
    };

    void MoCSweeper_2D3D::sweep( int group ) {
        assert(source_);

        // set up the xstr_ array
        for( auto &xsr: *xs_mesh_ ) {
            real_t xstr = xsr.xsmactr()[group];
            for( auto &ireg: xsr.reg() ) {
                xstr_[ireg] = xstr;
            }
        }

        flux_1g_.reference( flux_( blitz::Range::all(), group ) );

        // Perform inner iterations
        for( unsigned int inner=0; inner<n_inner_; inner++ ) {
            // update the self-scattering source
            source_->self_scatter( group );

            // Perform the stock sweep unless we are on the last outer and have
            // a CoarseData object.
            if( inner == n_inner_-1 && coarse_data_ ) {
                coarse_data_->zero_data_radial( group );
                //coarse_data_->current(blitz::Range::all(), group) = 0.0;
                cmdo::CurrentCorrections cw( coarse_data_, &mesh_,
                        corrections_.get(), source_->get_transport( 0 ), xstr_,
                        ang_quad_, *sn_xs_mesh_, rays_ );
                this->sweep1g( group, cw );
                coarse_data_->set_has_radial_data(true);
            } else {
                moc::NoCurrent cw( coarse_data_, &mesh_ );
                this->sweep1g( group, cw );
            }
        }

        return;
    }
}