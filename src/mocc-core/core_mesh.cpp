#include "core_mesh.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>

#include "files.hpp"
#include "error.hpp"
#include "string_utils.hpp"


using std::cout;
using std::endl;
using std::stringstream;


namespace mocc {
    CoreMesh::CoreMesh(pugi::xml_node &input) {
        // Parse meshes
        for (pugi::xml_node mesh = input.child( "mesh" );
             mesh; 
             mesh = mesh.next_sibling( "mesh" )) {
            LogFile << "Parsing new pin mesh: ID=" 
            << mesh.attribute( "id" ).value() << endl;
            UP_PinMesh_t pm( PinMeshFactory( mesh ) );
            int id = pm->id();
            pin_meshes_.emplace(id, std::move( pm ) );
        }
 
        
        // Parse Material Library
        std::string matLibName = 
            input.child( "material_lib" ).attribute( "path" ).value();
        cout << "Found material library specification: " << matLibName << endl;
        FileScrubber matLibFile( matLibName.c_str(), "!" );
        mat_lib_ = MaterialLib( matLibFile );
        
        // Parse material IDs
        for (pugi::xml_node mat = input.child( "material_lib" ).
                child( "material" ); 
                mat; mat = mat.next_sibling( "material" )){
            cout << mat.attribute( "id" ).value() << " "
                 << mat.attribute( "name" ).value() << endl;
            mat_lib_.assignID( mat.attribute( "id" ).as_int(),
                               mat.attribute( "name" ).value() );
        }
        
        // Parse pins
        for ( pugi::xml_node pin = input.child( "pin" ); pin; 
                pin = pin.next_sibling( "pin" ) ) {
            
            // Construct the pin and add to the map
            UP_Pin_t pin_p( new Pin( pin, pin_meshes_ ) );
            pins_.emplace( pin_p->id(), std::move(pin_p) );
        }

        // Parse lattices
        for ( pugi::xml_node lat = input.child( "lattice" ); lat;
                lat = lat.next_sibling( "lattice" )) {
            Lattice lattice( lat, pins_ );
            lattices_.emplace( lattice.id(), lattice );
        }

        // Parse assemblies
        for ( pugi::xml_node asy = input.child("assembly"); asy;
                asy = asy.next_sibling("assembly") ) {
            UP_Assembly_t asy_p( new Assembly( asy, lattices_ ) );
            assemblies_.emplace( asy_p->id(), std::move(asy_p) );
        }

        // Parse core
        core_ = Core( input.child("core"), assemblies_ );

        nx_ = core_.nx();
        ny_ = core_.ny();
        nz_ = core_.nz();
        nasy_ = nx_*ny_;

        // Calculate the total core dimensions
        hx_ = 0.0;
        for ( int ix=0; ix<core_.nx(); ix++ ) {
            hx_ += core_.at(ix, 0).hx();
        }
        hy_ = 0.0;
        for ( int iy=0; iy<core_.ny(); iy++ ) {
            hy_ += core_.at(iy, 0).hy();
        }

        // Determine the set of geometricaly-unique axial planes
        std::vector< VecI > unique;
        VecI plane_pins;
        int plane_reg = 0;
        for ( unsigned int iz=0; iz<nz_; iz++) {
            first_reg_plane_.push_back(plane_reg);
            // Form a list of all pin meshes in the core plane iz
            for ( unsigned int iasy=0; iasy<nasy_; iasy++ ) {
                const Assembly& asy = core_.at(iasy);
                for ( auto &pin: asy[iz] ) {
                    plane_pins.push_back(pin->mesh_id());
                    core_pins_.push_back(pin);
                    plane_reg += pin->n_reg();
                }
            }
            // Check against current list of unique planes
            int match_plane = -1;
            for( unsigned int iiz=0; iiz<unique.size(); iiz++ ) {
                for( unsigned int ip=0; ip<plane_pins.size(); ip++ ) {
                    if( plane_pins[ip] != unique[iiz][ip] ) {
                        // we dont have a match
                        break;
                    }
                    if ( ip == plane_pins.size()-1 ) {
                        // Looks like all of the pins matched!
                        match_plane = iiz;
                    }
                }
                if ( match_plane != -1 ) {
                    break;
                }
            }
            if ( match_plane == -1 ) {
                // This plane is thus far unique.
                unique.push_back( plane_pins );
                // Create a Plane instance for this collection of Lattices
                std::vector<const Lattice*> lattices;
                for (int ilat=0; ilat<core_.nx()*core_.ny(); ilat++) {
                    const Lattice* lat = &core_.at(ilat)[iz];
                    lattices.push_back(lat);
                }
                planes_.emplace_back(lattices, core_.nx(), core_.ny());
                unique_plane_.push_back( planes_.size() - 1 );
                first_unique_.push_back( iz );
            } else {
                // We did find a match to a previous plane. Push that ID
                unique_plane_.push_back( match_plane );
            }
            plane_pins.clear();
        } // Unique plane search

        // Put together the list of pin boundaries. For now we are treating them
        // as independent of axial plane
        x_vec_.push_back(0.0);
        float_t h_prev = 0.0;
        for( int ilatx=0; ilatx < core_.nx(); ilatx++ ) {
            const Assembly* asy = &core_.at(ilatx, 0);
            const Lattice* lat = &((*asy)[0]);
            for( auto &h: lat->hx_vec() ) {
                x_vec_.push_back(h + h_prev);
                lines_.push_back( Line( Point2(h+h_prev, 0.0), 
                                        Point2(h+h_prev, hy_) ) );
                h_prev += h;
            }
        }
        y_vec_.push_back(0.0);
        h_prev = 0.0;
        for( int ilaty=0; ilaty < core_.ny(); ilaty++ ) {
            const Assembly* asy = &core_.at(0, ilaty);
            const Lattice* lat = &((*asy)[0]);
            for( auto &h: lat->hy_vec() ) {
                y_vec_.push_back(h + h_prev);
                lines_.push_back( Line( Point2(0.0, h+h_prev), 
                                        Point2(hx_, h+h_prev) ) );
                h_prev += h;
            }
        }

        // Add up the number of regions and XS regions in the entire problem
        // geometry
        n_reg_   = 0;
        n_xsreg_ = 0;
        for ( auto &a: core_.assemblies() ) {
            n_reg_ += a->n_reg();
            n_xsreg_ += a->n_xsreg();
        }

        return;
    } // constructor

    CoreMesh::~CoreMesh() {
        return;
    }

    void CoreMesh::trace( std::vector<Point2> &ps ) const {
        assert(ps.size() == 2);
        Point2 p1 = ps[0];
        Point2 p2 = ps[1];

        Line l(p1, p2);

        for( auto &li: lines_ ) {
            Point2 intersection;
            if( Intersect(li, l, intersection) == 1 ) {
                ps.push_back( intersection );
            }
        }

        // Sort the points and remove duplicates
        std::sort(ps.begin(), ps.end());
        ps.erase( std::unique(ps.begin(), ps.end()), ps.end() );

        return;
    }

    const PinMesh* CoreMesh::get_pinmesh( Point2 &p, unsigned int iz, 
            int &first_reg ) const {
        assert( (iz >= 0) & (iz<planes_.size()) );
        return planes_[iz].get_pinmesh(p, first_reg);
    }
}
