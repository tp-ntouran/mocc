#pragma once

#include <memory>

#include "pugixml.hpp"

#include "blitz_typedefs.hpp"
#include "eigen_interface.hpp"
#include "h5file.hpp"
#include "xs_mesh.hpp"

namespace mocc {
    /**
     * This class provides a homogenized version of an \ref XSMesh. At the time
     * of its conception, it was mostly intended for use with the Sn sweeper and
     * for future application to CMFD acceleration, so a lot of the code and
     * documentation make that assumption. If this is generalized much in the
     * future, make sure to update the docs.
     */
    class XSMeshHomogenized: public XSMesh {
    public:
        /** 
         * Construct a homogenized \ref XSMesh from only a \ref CoreMesh object.
         * Following construction, the cross sections in the mesh will be
         * volume-weighted.
         */
        XSMeshHomogenized( const CoreMesh& mesh );

        /**
         * Construct a homogenized \ref XSMesh from a \ref CoreMesh and external
         * data. Following construction, the cross sections will be those
         * specified by the auxiliary data files specified by the \p input XML.
         * 
         * \param mesh a \ref CoreMesh with which the \ref XSMeshHomogenized
         * should conform.
         * \param input an XML node containing at least one \<data\> child
         */
        XSMeshHomogenized( const CoreMesh& mesh, const pugi::xml_node &input );

        /**
         * Update homogenized cross sections using the passed flux array
         */
        void update();

        /**
         * \brief Associate the homogenized cross-section mesh with a flux array
         * for flux-volume weighting.
         *
         * This associates the internal \c flux_ pointer with a fine mesh flux
         * array for use in the \ref XSMeshHomogenized::update() method. If no
         * flux has been associated when \ref update() is called, the update is
         * skipped, preserving the volume-weighted cross sections, which are
         * calculated at construction time.
         */
        void set_flux( const ArrayB2 &flux ) {
            assert( flux.extent(0) == (int)mesh_.n_reg() );
            flux_ = &flux;
        }

        /**
         * Generate output of important cross sections on the homogenized mesh
         */
        void output( H5::CommonFG *file ) const;
    private:
        const CoreMesh& mesh_;

        // Possibly-associated flux for homogenization.
        const ArrayB2 *flux_;

        /**
        * \brief Return an XSMeshRegion containing homogenized cross sections
        * from a pin cell. No flux wieghting is performed, only volume
        * weighting.
        */
        void homogenize_region( int i, const Pin& pin, 
                XSMeshRegion &xsr ) const;

        /**
        * \brief Return an XSMeshRegion containing homogenized cross sections
        * from a pin cell. Use the passed scalar flux to perform flux-volume
        * weighting.
        *
        * \param i the region in the Sn or coarse mesh that the region should
        * belong to. Since it is assumed that there is a one-to-one mapping from
        * the xs mesh to the Sn mesh, the vector of FSRs in the \ref
        * XSMeshRegion will only contain one element, populated with the value
        * of \c i.
        * \param first_reg the region offset into the \c flux array to be used
        * for this particular pin.
        * \param pin the pin to homogenize cross sections for.
        * \param xsr the \ref XSMeshRegion into which the cross sections should
        * be homogenized.
        *
        * This routine performs a flux-weighted homogenization of the cross
        * sections in the passed \ref Pin object and returns an \ref
        * XSMeshRegion object containing the homogenized cross sections.
        */
        void homogenize_region_flux( int i, int first_reg, const Pin& pin, 
                XSMeshRegion &xsr ) const;
    };

    typedef std::shared_ptr<XSMeshHomogenized> SP_XSMeshHomogenized_t;
}
