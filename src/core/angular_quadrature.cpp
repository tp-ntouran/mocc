#include "angular_quadrature.hpp"

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

#include "core/error.hpp"
#include "core/string_utils.hpp"
#include "core/level_symmetric.hpp"


namespace mocc {

    const int AngularQuadrature::reflection_[3][8] = { {1,0,3,2,5,4,7,6},
                                                       {3,2,1,0,7,6,5,4},
                                                       {4,5,6,7,0,1,2,3} };

    AngularQuadrature::AngularQuadrature( const pugi::xml_node &input ) {
        // Make sure we got input
        if( input.empty() ) {
            throw EXCEPT("No input provided for angular quadrature.");
        }
        if( input.name() != std::string("ang_quad") ) {
            std::cerr << input.name() << std::endl;
            throw EXCEPT("Input is not an <ang_quad/> tag");
        }

        // Extract the quadrature type
        std::string type_str = input.attribute("type").value();
        sanitize(type_str);
        if ( (type_str == "ls") || (type_str == "level-symmetric") ) {
            type_ = QuadratureType::LS;

            // extract the quadrature order
            int order = input.attribute("order").as_int(-1);

            // Generate angles for octant 1
            angles_ = GenSn( order );
        } else if ((type_str == "cg") || (type_str == "chebyshev-gaussian" )) {
            type_ = QuadratureType::CHEB_GAUSS;

            //extract the quadrature order
            int azi_order = input.attribute("azimuthal-order").as_int(-1);
            int pol_order = input.attribute("polar-order").as_int(-1);

            //Generate angles for octant 1
            angles_ = GenProduct(GenChebyshev(azi_order),GenGauss(pol_order));
        } else if ((type_str == "cy") || (type_str == "chebyshev-yamamoto" )) {
            type_ = QuadratureType::CHEB_YAMAMOTO;

            //extract the quadrature order
            int azi_order = input.attribute("azimuthal-order").as_int(-1);
            int pol_order = input.attribute("polar-order").as_int(-1);

            //Generate angles from octant 1
            angles_ = GenProduct(GenChebyshev(azi_order),GenYamamoto(pol_order));
        } else {
            std::cerr << "'" << type_str << "'" << std::endl;
            throw EXCEPT("Unrecognized angular quadrature type specified.");
        }

        // Store the number of angles per octant
        ndir_oct_ = angles_.size();

        // Expand angles to other octants
        for ( int ioct=2; ioct<=8; ioct++ ) {
            for ( int iang=0; iang < ndir_oct_; iang++ ) {
                Angle a = angles_[iang];
                angles_.push_back( a.to_octant(ioct) );
            }
        }

        return;
    }

    AngularQuadrature::AngularQuadrature( const H5Node &input ) {
        VecF ox;
        VecF oy;
        VecF oz;
        VecF weights;

        input.read("ang_quad/omega_x", ox);
        input.read("ang_quad/omega_y", oy);
        input.read("ang_quad/omega_z", oz);
        input.read("ang_quad/weight", weights);

        if( (ox.size() != oy.size()) || (ox.size() != oz.size()) || 
                (ox.size() != weights.size()) ) {
            throw EXCEPT("Incompatible data sizes");
        }

        int size = ox.size();
        if( size%8 != 0 ) {
            throw EXCEPT("Size is not evenly-divisible by 8");
        }

        ndir_oct_ = size/8;

        angles_.reserve(size);
        for( int iang=0; iang<size; iang++ ) {
            angles_.emplace_back(ox[iang], oy[iang], oz[iang], weights[iang]);
        }

        type_ = QuadratureType::MANUAL;
        
        return;
    }

    void AngularQuadrature::modify_angle(int iang, Angle ang ) {
        for ( int ioct=0; ioct<8; ioct++ ){
            angles_[iang + ioct*ndir_oct_] = ang.to_octant(ioct+1);
        }
    }

    std::ostream& operator<<(std::ostream& os,
            const AngularQuadrature &angquad) {
        const int w = 12;
        os << std::setw(w) << "Alpha"
           << std::setw(w) << "Theta"
           << std::setw(w) << "omega x"
           << std::setw(w) << "omega y"
           << std::setw(w) << "omega z" << std::endl;
        for( auto &ang: angquad.angles_ ) {
            os << ang << std::endl;
        }
        return os;
    }

    void AngularQuadrature::output( H5Node &node ) const {
        VecF alpha; alpha.reserve(this->ndir());
        VecF theta; theta.reserve(this->ndir());
        VecF ox;    ox.reserve(this->ndir());
        VecF oy;    oy.reserve(this->ndir());
        VecF oz;    oz.reserve(this->ndir());
        VecF w;     w.reserve(this->ndir());
        
        for( auto a: angles_ ) {
            alpha.push_back(a.alpha);
            theta.push_back(a.theta);
            ox.push_back(a.ox);
            oy.push_back(a.oy);
            oz.push_back(a.oz);
            w.push_back(a.weight);
        }

        auto g = node.create_group("ang_quad");

        g.write("alpha", alpha);
        g.write("theta", theta);
        g.write("omega_x", ox);
        g.write("omega_y", oy);
        g.write("omega_z", oz);
        g.write("weight", w);

        return;
    }
}
