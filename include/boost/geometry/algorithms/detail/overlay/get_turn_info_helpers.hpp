// Boost.Geometry (aka GGL, Generic Geometry Library)

// Copyright (c) 2007-2012 Barend Gehrels, Amsterdam, the Netherlands.

// This file was modified by Oracle on 2013, 2014, 2015, 2017.
// Modifications copyright (c) 2013-2017 Oracle and/or its affiliates.

// Contributed and/or modified by Adam Wulkiewicz, on behalf of Oracle

// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_HELPERS_HPP
#define BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_HELPERS_HPP

#include <boost/geometry/policies/robustness/no_rescale_policy.hpp>
#include <boost/geometry/algorithms/detail/direction_code.hpp>

namespace boost { namespace geometry {

#ifndef DOXYGEN_NO_DETAIL
namespace detail { namespace overlay {

enum turn_position { position_middle, position_front, position_back };

template <typename Point, typename SegmentRatio>
struct turn_operation_linear
    : public turn_operation<Point, SegmentRatio>
{
    turn_operation_linear()
        : position(position_middle)
        , is_collinear(false)
    {}

    turn_position position;
    bool is_collinear; // valid only for Linear geometry
};

template
<
    typename TurnPointCSTag,
    typename UniqueSubRange1, typename UniqueSubRange2,
    typename SideStrategy
>
struct side_calculator
{
    typedef typename UniqueSubRange1::point_type PointP;
    typedef typename UniqueSubRange2::point_type PointQ;

    inline side_calculator(UniqueSubRange1 const& range_p,
                           UniqueSubRange2 const& range_q,
                           SideStrategy const& side_strategy)
        : m_side_strategy(side_strategy)
        , m_range_p(range_p)
        , m_range_q(range_q)
    {}

    inline int pk_wrt_p1() const { return m_side_strategy.apply(get_pi(), get_pj(), get_pk()); }
    inline int pk_wrt_q1() const { return m_side_strategy.apply(get_qi(), get_qj(), get_pk()); }
    inline int qk_wrt_p1() const { return m_side_strategy.apply(get_pi(), get_pj(), get_qk()); }
    inline int qk_wrt_q1() const { return m_side_strategy.apply(get_qi(), get_qj(), get_qk()); }

    inline int pk_wrt_q2() const { return m_side_strategy.apply(get_qj(), get_qk(), get_pk()); }
    inline int qk_wrt_p2() const { return m_side_strategy.apply(get_pj(), get_pk(), get_qk()); }

    inline PointP const& get_pi() const { return m_range_p.at(0); }
    inline PointP const& get_pj() const { return m_range_p.at(1); }
    inline PointP const& get_pk() const { return m_range_p.at(2); }

    inline PointQ const& get_qi() const { return m_range_q.at(0); }
    inline PointQ const& get_qj() const { return m_range_q.at(1); }
    inline PointQ const& get_qk() const { return m_range_q.at(2); }

    SideStrategy m_side_strategy; // NOTE: cannot be const&
    UniqueSubRange1 const& m_range_p;
    UniqueSubRange2 const& m_range_q;
};

template<typename Point, typename UniqueSubRange, typename RobustPolicy>
struct robust_subrange_adapter
{
    typedef Point point_type;

    robust_subrange_adapter(UniqueSubRange const& unique_sub_range,
                     Point const& robust_point_i, Point const& robust_point_j,
                     RobustPolicy const& robust_policy)

        : m_unique_sub_range(unique_sub_range)
        , m_robust_policy(robust_policy)
        , m_robust_point_i(robust_point_i)
        , m_robust_point_j(robust_point_j)
        , m_k_retrieved(false)
    {}

    std::size_t size() const { return m_unique_sub_range.size(); }

    //! Get precalculated point
    Point const& at(std::size_t index) const
    {
        switch (index)
        {
            case 0 : return m_robust_point_i;
            case 1 : return m_robust_point_j;
            case 2 : return get_point_k();
            default : return m_robust_point_i;
        }
    }

private :
    Point const& get_point_k() const
    {
        if (! m_k_retrieved)
        {
            geometry::recalculate(m_robust_point_k, m_unique_sub_range.at(2), m_robust_policy);
            m_k_retrieved = true;
        }
        return m_robust_point_k;
    }

    UniqueSubRange const& m_unique_sub_range;
    RobustPolicy const& m_robust_policy;

    Point const& m_robust_point_i;
    Point const& m_robust_point_j;
    mutable Point m_robust_point_k;

    mutable bool m_k_retrieved;
};

template
<
    typename UniqueSubRange1, typename UniqueSubRange2,
    typename RobustPolicy
>
struct robust_points
{
    typedef typename geometry::robust_point_type
        <
            typename UniqueSubRange1::point_type, RobustPolicy
        >::type robust_point1_type;

    typedef robust_point1_type robust_point2_type;

    inline robust_points(UniqueSubRange1 const& range_p,
                         UniqueSubRange2 const& range_q,
                         RobustPolicy const& robust_policy)
        : m_robust_policy(robust_policy)
        , m_range_p(range_p)
        , m_range_q(range_q)
        , m_pk_retrieved(false)
        , m_qk_retrieved(false)
    {
        // Calculate pi,pj and qi,qj which are almost always necessary
        // But don't calculate pk/qk yet, which is retrieved (taking
        // more time) and not always necessary.
        geometry::recalculate(m_rpi, range_p.at(0), robust_policy);
        geometry::recalculate(m_rpj, range_p.at(1), robust_policy);
        geometry::recalculate(m_rqi, range_q.at(0), robust_policy);
        geometry::recalculate(m_rqj, range_q.at(1), robust_policy);
    }

    inline robust_point1_type const& get_rpk() const
    {
        if (! m_pk_retrieved)
        {
            geometry::recalculate(m_rpk, m_range_p.at(2), m_robust_policy);
            m_pk_retrieved = true;
        }
        return m_rpk;
    }
    inline robust_point2_type const& get_rqk() const
    {
        if (! m_qk_retrieved)
        {
            geometry::recalculate(m_rqk, m_range_q.at(2), m_robust_policy);
            m_qk_retrieved = true;
        }
        return m_rqk;
    }

    robust_point1_type m_rpi, m_rpj;
    robust_point2_type m_rqi, m_rqj;

private :
    RobustPolicy const& m_robust_policy;
    UniqueSubRange1 const& m_range_p;
    UniqueSubRange2 const& m_range_q;

    // On retrieval
    mutable robust_point1_type m_rpk;
    mutable robust_point2_type m_rqk;
    mutable bool m_pk_retrieved;
    mutable bool m_qk_retrieved;
};

template
<
    typename UniqueSubRange1, typename UniqueSubRange2,
    typename TurnPoint, typename IntersectionStrategy, typename RobustPolicy>
class intersection_info_base
    : private robust_points<UniqueSubRange1, UniqueSubRange2, RobustPolicy>
{
    typedef robust_points<UniqueSubRange1, UniqueSubRange2, RobustPolicy> base;

public:
    typedef typename base::robust_point1_type robust_point1_type;
    typedef typename base::robust_point2_type robust_point2_type;

    typedef robust_subrange_adapter<robust_point1_type, UniqueSubRange1, RobustPolicy> robust_subrange1;
    typedef robust_subrange_adapter<robust_point2_type, UniqueSubRange2, RobustPolicy> robust_subrange2;

    typedef typename cs_tag<TurnPoint>::type cs_tag;

    typedef typename IntersectionStrategy::side_strategy_type side_strategy_type;
    typedef side_calculator<cs_tag, robust_subrange1, robust_subrange2,
             side_strategy_type> side_calculator_type;

    typedef side_calculator
        <
            cs_tag, robust_subrange2, robust_subrange1,
            side_strategy_type
        > robust_swapped_side_calculator_type;

    intersection_info_base(UniqueSubRange1 const& range_p,
                           UniqueSubRange2 const& range_q,
                           IntersectionStrategy const& intersection_strategy,
                           RobustPolicy const& robust_policy)
        : base(range_p, range_q, robust_policy)
        , m_range_p(range_p)
        , m_range_q(range_q)
        , m_robust_range_p(range_p, base::m_rpi, base::m_rpj, robust_policy)
        , m_robust_range_q(range_q, base::m_rqi, base::m_rqj, robust_policy)
        , m_side_calc(m_robust_range_p, m_robust_range_q,
                      intersection_strategy.get_side_strategy())
    {}

    inline typename UniqueSubRange1::point_type const& pi() const { return m_range_p.at(0); }
    inline typename UniqueSubRange2::point_type const& qi() const { return m_range_q.at(0); }

    inline robust_point1_type const& rpi() const { return base::m_rpi; }
    inline robust_point1_type const& rpj() const { return base::m_rpj; }
    inline robust_point1_type const& rpk() const { return base::get_rpk(); }

    inline robust_point2_type const& rqi() const { return base::m_rqi; }
    inline robust_point2_type const& rqj() const { return base::m_rqj; }
    inline robust_point2_type const& rqk() const { return base::get_rqk(); }

    inline side_calculator_type const& sides() const { return m_side_calc; }
    
    robust_swapped_side_calculator_type get_swapped_sides() const
    {
        robust_swapped_side_calculator_type result(
                            m_robust_range_q, m_robust_range_p,
                            m_side_calc.m_side_strategy);
        return result;
    }

    UniqueSubRange1 const& m_range_p;
    UniqueSubRange2 const& m_range_q;
private :
    robust_subrange1 m_robust_range_p;
    robust_subrange2 m_robust_range_q;
    side_calculator_type m_side_calc;
};

template
<
    typename UniqueSubRange1, typename UniqueSubRange2,
    typename TurnPoint, typename IntersectionStrategy
>
class intersection_info_base<UniqueSubRange1, UniqueSubRange2,
        TurnPoint, IntersectionStrategy, detail::no_rescale_policy>
{
public:
    typedef typename UniqueSubRange1::point_type point1_type;
    typedef typename UniqueSubRange2::point_type point2_type;

    typedef typename UniqueSubRange1::point_type robust_point1_type;
    typedef typename UniqueSubRange2::point_type robust_point2_type;

    typedef typename cs_tag<TurnPoint>::type cs_tag;

    typedef typename IntersectionStrategy::side_strategy_type side_strategy_type;
    typedef side_calculator<cs_tag, UniqueSubRange1, UniqueSubRange2, side_strategy_type> side_calculator_type;

    typedef side_calculator
        <
            cs_tag, UniqueSubRange2, UniqueSubRange1,
            side_strategy_type
        > swapped_side_calculator_type;
    
    intersection_info_base(UniqueSubRange1 const& range_p,
                           UniqueSubRange2 const& range_q,
                           IntersectionStrategy const& intersection_strategy,
                           no_rescale_policy const& /*robust_policy*/)
        : m_range_p(range_p)
        , m_range_q(range_q)
        , m_side_calc(range_p, range_q,
                      intersection_strategy.get_side_strategy())
    {}

    inline point1_type const& pi() const { return m_side_calc.get_pi(); }
    inline point2_type const& qi() const { return m_side_calc.get_qi(); }

    inline point1_type const& rpi() const { return m_side_calc.get_pi(); }
    inline point1_type const& rpj() const { return m_side_calc.get_pj(); }
    inline point1_type const& rpk() const { return m_side_calc.get_pk(); }

    inline point2_type const& rqi() const { return m_side_calc.get_qi(); }
    inline point2_type const& rqj() const { return m_side_calc.get_qj(); }
    inline point2_type const& rqk() const { return m_side_calc.get_qk(); }

    inline side_calculator_type const& sides() const { return m_side_calc; }

    swapped_side_calculator_type get_swapped_sides() const
    {
        swapped_side_calculator_type result(
            m_range_q, m_range_p,
            m_side_calc.m_side_strategy);
        return result;
    }

protected :
    UniqueSubRange1 const& m_range_p;
    UniqueSubRange2 const& m_range_q;
private :
    side_calculator_type m_side_calc;
};


template
<
    typename UniqueSubRange1, typename UniqueSubRange2,
    typename TurnPoint,
    typename IntersectionStrategy,
    typename RobustPolicy
>
class intersection_info
    : public intersection_info_base<UniqueSubRange1, UniqueSubRange2,
        TurnPoint, IntersectionStrategy, RobustPolicy>
{
    typedef intersection_info_base<UniqueSubRange1, UniqueSubRange2,
        TurnPoint, IntersectionStrategy, RobustPolicy> base;

public:
    typedef segment_intersection_points
    <
        TurnPoint,
        typename geometry::segment_ratio_type
            <
                TurnPoint, RobustPolicy
            >::type
    > intersection_point_type;

    typedef typename UniqueSubRange1::point_type point1_type;
    typedef typename UniqueSubRange2::point_type point2_type;

    // NOTE: formerly defined in intersection_strategies
    typedef policies::relate::segments_tupled
        <
            policies::relate::segments_intersection_points
                <
                    intersection_point_type
                >,
            policies::relate::segments_direction
        > intersection_policy_type;

    typedef IntersectionStrategy intersection_strategy_type;
    typedef typename IntersectionStrategy::side_strategy_type side_strategy_type;

    typedef model::referring_segment<point1_type const> segment_type1;
    typedef model::referring_segment<point2_type const> segment_type2;
    typedef typename base::side_calculator_type side_calculator_type;
    
    typedef typename intersection_policy_type::return_type result_type;
    typedef typename boost::tuples::element<0, result_type>::type i_info_type; // intersection_info
    typedef typename boost::tuples::element<1, result_type>::type d_info_type; // dir_info

    intersection_info(UniqueSubRange1 const& range_p,
                      UniqueSubRange2 const& range_q,
                      IntersectionStrategy const& intersection_strategy,
                      RobustPolicy const& robust_policy)
        : base(range_p, range_q,
               intersection_strategy, robust_policy)
        , m_result(intersection_strategy.apply(
                        segment_type1(range_p.at(0), range_p.at(1)),
                        segment_type2(range_q.at(0), range_q.at(1)),
                        intersection_policy_type(),
                        robust_policy,
                        base::rpi(), base::rpj(),
                        base::rqi(), base::rqj()))
        , m_intersection_strategy(intersection_strategy)
        , m_robust_policy(robust_policy)
    {}

    inline result_type const& result() const { return m_result; }
    inline i_info_type const& i_info() const { return m_result.template get<0>(); }
    inline d_info_type const& d_info() const { return m_result.template get<1>(); }

    inline side_strategy_type get_side_strategy() const
    {
        return m_intersection_strategy.get_side_strategy();
    }

    // TODO: it's more like is_spike_ip_p
    inline bool is_spike_p() const
    {
        if (base::m_range_p.size() == 2u)
        {
            return false;
        }
        if (base::sides().pk_wrt_p1() == 0)
        {
            // p:  pi--------pj--------pk
            // or: pi----pk==pj

            if (! is_ip_j<0>())
            {
                return false;
            }

            int const qk_p1 = base::sides().qk_wrt_p1();
            int const qk_p2 = base::sides().qk_wrt_p2();

            if (qk_p1 == -qk_p2)
            {
                if (qk_p1 == 0)
                {
                    // qk is collinear with both p1 and p2,
                    // verify if pk goes backwards w.r.t. pi/pj
                    return direction_code(base::rpi(), base::rpj(), base::rpk()) == -1;
                }

                // qk is at opposite side of p1/p2, therefore
                // p1/p2 (collinear) are opposite and form a spike
                return true;
            }
        }
        
        return false;
    }

    inline bool is_spike_q() const
    {
        if (base::m_range_q.size() == 2u)
        {
            return false;
        }

        // See comments at is_spike_p
        if (base::sides().qk_wrt_q1() == 0)
        {
            if (! is_ip_j<1>())
            {
                return false;
            }

            int const pk_q1 = base::sides().pk_wrt_q1();
            int const pk_q2 = base::sides().pk_wrt_q2();
                
            if (pk_q1 == -pk_q2)
            {
                if (pk_q1 == 0)
                {
                    return direction_code(base::rqi(), base::rqj(), base::rqk()) == -1;
                }
                        
                return true;
            }
        }
        
        return false;
    }

private:
    template <std::size_t OpId>
    bool is_ip_j() const
    {
        int arrival = d_info().arrival[OpId];
        bool same_dirs = d_info().dir_a == 0 && d_info().dir_b == 0;

        if (same_dirs)
        {
            if (i_info().count == 2)
            {
                return arrival != -1;
            }
            else
            {
                return arrival == 0;
            }
        }
        else
        {
            return arrival == 1;
        }
    }

    result_type m_result;
    IntersectionStrategy const& m_intersection_strategy;
    RobustPolicy const& m_robust_policy;
};

}} // namespace detail::overlay
#endif // DOXYGEN_NO_DETAIL

}} // namespace boost::geometry

#endif // BOOST_GEOMETRY_ALGORITHMS_DETAIL_OVERLAY_GET_TURN_INFO_HELPERS_HPP
