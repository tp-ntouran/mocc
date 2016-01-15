#pragma once

#include <memory>
#include <vector>
#include <map>
#include <iostream>

#include "pugixml.hpp"

#include "pin.hpp"
#include "geom.hpp"
#include "pin_mesh_base.hpp"
#include "global_config.hpp"

namespace mocc {
    class Lattice {
    public:
        Lattice( const pugi::xml_node &input,
                 const std::map<int, UP_Pin_t> &pins );

        size_t id() const {
            return id_;
        }

        /**
         * \brief Number of pins in the x direction
         */
        size_t nx() const {
            return nx_;
        }

        /**
         * \brief Number of pins in the y direction
         */
        size_t ny() const {
            return ny_;
        }

        /**
         * \brief Total number of pins in the lattice
         */
        size_t n_pin() const {
            return pins_.size();
        }

        /**
         * \brief Return the size of the lattice along the x dimension
         */
        real_t hx() const {
            return hx_;
        }

        /**
         * \brief Return the size of the lattice along the y dimension
         */
        real_t hy() const {
            return hy_;
        }

        /**
         * \brief Return a const reference to the \ref Pin located at the given
         * location of the \ref Lattice.
         */
        const Pin& at( size_t x, size_t y ) const {
            assert( (0 <= x) & (x < nx_) );
            assert( (0 <= y) & (y < ny_) );
            return *pins_[y*nx_ + x];
        }

        /**
         * \brief Return an iterator to the first \ref Pin* in the \ref Lattice
         */
        std::vector<Pin*>::const_iterator begin() const {
            return pins_.cbegin();
        }

        /**
         * \brief Return an iterator past the last Pin* in the lattice
         */
        std::vector<Pin*>::const_iterator end() const {
            return pins_.cend();
        }

        /**
         * \brief Return a const reference to the underlying hx_vec_ array
         */
        const VecF& hx_vec() const {
            return hx_vec_;
        }

        /**
         * \brief Return a const reference to the underlying hy_vec_ array
         */
        const VecF& hy_vec() const {
            return hy_vec_;
        }

        /**
         * \brief Return the total number of regions in the lattice
         */
        size_t n_reg() const {
            return n_reg_;
        }

        /**
         * \brief Return the total number of XS regions in the lattice
         */
        size_t n_xsreg() const {
            return n_xsreg_;
        }

        /**
         * Return a const reference to the Pin Mesh object located at the
         * provided point and increment the passed in first_reg by the pin's
         * first-region offset. These calls are chained from the CoreMesh ->
         * Plane -> Lattice, with each level in the geometrical hierarchy moving
         * the point to the appropriate local coordinates and offsetting the
         * first_reg value.
         */
        const PinMesh* get_pinmesh( Point2 &p, int &first_reg ) const;

        /**
         * \brief Return whether the current \ref Lattice and the passed \ref
         * Lattice reference are \ref Assembly compatible.
         *
         * \ref Assembly compatible means that the two \ref Lattice's can be
         * stacked atop each other and their pin boundaries will line up.
         */
        bool compatible( const Lattice &other ) const;

    private:
        size_t id_;
        size_t nx_;
        size_t ny_;
        size_t n_reg_;
        size_t n_xsreg_;
        real_t hx_;
        real_t hy_;
        VecF hx_vec_;
        VecF hy_vec_;
        VecF x_vec_;
        VecF y_vec_;

        // Array of pins in the lattice
        std::vector<Pin*> pins_;

        // Array of starting FSR indices for each pin in the lattice
        VecI first_reg_pin_;
    };

    typedef std::shared_ptr<Lattice> SP_Lattice_t;
    typedef std::shared_ptr<Lattice> UP_Lattice_t;

    std::map<int, UP_Lattice_t> ParseLattices( const pugi::xml_node &input,
            const std::map<int, UP_Pin_t> &pins );

}