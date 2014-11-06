#include "geolib/sensors/LaserRangeFinder.h"
#include "geolib/Shape.h"

namespace geo {

LaserRangeFinder::LaserRangeFinder() : a_min_(0), a_max_(0), num_beams_(0) {
}

LaserRangeFinder::~LaserRangeFinder() {
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//
//                                        RENDERING
//
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

// ----------------------------------------------------------------------------------------------------

void LaserRangeFinder::RenderResult::renderLine(const Vector3& p1, const Vector3& p2)
{

    int i_p1 = lrf_->getAngleUpperIndex(p1.x, p1.y);
    int i_p2 = lrf_->getAngleUpperIndex(p2.x, p2.y);

    int i_min = std::min(i_p1, i_p2);
    int i_max = std::max(i_p1, i_p2);

    int i_min1, i_max1, i_min2, i_max2;

    if (i_max - i_min < lrf_->i_half_circle_)
    {
        // Both points in LRF blind spot, so don't render a line
        if (i_min >= lrf_->num_beams_)
            return;

        i_min1 = i_min;
        i_max1 = std::min(lrf_->num_beams_, i_max);

        // No second part
        i_min2 = 0;
        i_max2 = 0;

        min_i = std::min(min_i, (int)i_min);
        max_i = std::max(max_i, (int)i_max);
    }
    else
    {
        i_min1 = i_max;
        i_max1 = lrf_->num_beams_;

        i_min2 = 0;
        i_max2 = std::min(lrf_->num_beams_, i_min);

        min_i = 0;
        max_i = lrf_->num_beams_;
    }

    // d = (q1 - ray_start) x s / (r x s)
    //   = (q1 x s) / (r x s)
    Vector3 s = p2 - p1;

    // Draw part 1
    for(int i = i_min1; i < i_max1; ++i)
    {
        const Vector3& r = lrf_->ray_dirs_[i];
        double d = (p1.x * s.y - p1.y * s.x) / (r.x * s.y - r.y * s.x);
        if (d > 0)
            renderPoint(i, d);
    }

    // Draw part 2
    for(int i = i_min2; i < i_max2; ++i)
    {
        const Vector3& r = lrf_->ray_dirs_[i];
        double d = (p1.x * s.y - p1.y * s.x) / (r.x * s.y - r.y * s.x);
        if (d > 0)
            renderPoint(i, d);
    }
}

// ----------------------------------------------------------------------------------------------------

void LaserRangeFinder::RenderResult::renderPoint(int i, float d)
{
    if (ranges[i] == 0 || d < ranges[i]) {
        ranges[i] = d;
    }
}

// ----------------------------------------------------------------------------------------------------

void LaserRangeFinder::render(const LaserRangeFinder::RenderOptions& opt, LaserRangeFinder::RenderResult& res) const {
    res.min_i = 0;
    res.max_i = ray_dirs_.size();

    if (res.ranges.size() != ray_dirs_.size()) {
        res.ranges.resize(ray_dirs_.size(), 0);
    }

    res.lrf_ = this;

    const geo::Pose3D& pose = opt.getPose();

    double max_radius = opt.getMesh().getMaxRadius();
    if (max_radius > 0) {

        // If object is to far above or below the laser plane, do not render
        if (std::abs(pose.getOrigin().getZ()) > max_radius) {
            return;
        }

        double dist_sq = pose.getOrigin().length2();

        if (dist_sq > max_radius * max_radius) {
            // If nearest object point is certainly further away than max_range, do not render
            if (sqrt(dist_sq) - max_radius > range_max_) {
                return;
            }
        }
    }

    const std::vector<TriangleI>& triangles = opt.getMesh().getTriangleIs();
    const std::vector<Vector3>& points = opt.getMesh().getPoints();

    // transform Z-coordinates
    std::vector<double> zs_t(points.size());
    Vector3 Rz = pose.getBasis().getRow(2);
    double z_offset = pose.getOrigin().getZ();
    for(unsigned int i = 0; i < points.size(); ++i) {
        zs_t[i] = Rz.dot(points[i]) + z_offset;
    }

    for(std::vector<TriangleI>::const_iterator it_tri = triangles.begin(); it_tri != triangles.end(); ++it_tri) {
        bool p1_under_plane = zs_t[it_tri->i1_] < 0;
        bool p2_under_plane = zs_t[it_tri->i2_] < 0;
        bool p3_under_plane = zs_t[it_tri->i3_] < 0;

        if (p1_under_plane != p2_under_plane || p2_under_plane != p3_under_plane) {

            Vector3 p1_3d = pose * points[it_tri->i1_];
            Vector3 p2_3d = pose * points[it_tri->i2_];
            Vector3 p3_3d = pose * points[it_tri->i3_];

            double z1 = std::abs(p1_3d.getZ());
            double z2 = std::abs(p2_3d.getZ());
            double z3 = std::abs(p3_3d.getZ());

            Vector3 q1, q2;
            if (p2_under_plane == p3_under_plane) {
                if (p2_under_plane)
                {
                    q2 = (p1_3d * z2 + p2_3d * z1) / (z1 + z2);
                    q1 = (p1_3d * z3 + p3_3d * z1) / (z1 + z3);
                }
                else
                {
                    q1 = (p1_3d * z2 + p2_3d * z1) / (z1 + z2);
                    q2 = (p1_3d * z3 + p3_3d * z1) / (z1 + z3);
                }
            } else if (p1_under_plane == p3_under_plane) {
                if (p1_under_plane)
                {
                    q1 = (p2_3d * z1 + p1_3d * z2) / (z2 + z1);
                    q2 = (p2_3d * z3 + p3_3d * z2) / (z2 + z3);
                }
                else
                {
                    q1 = (p2_3d * z3 + p3_3d * z2) / (z2 + z3);
                    q2 = (p2_3d * z1 + p1_3d * z2) / (z2 + z1);
                }
            } if (p1_under_plane == p2_under_plane) {
                if (p1_under_plane)
                {
                    q1 = (p3_3d * z2 + p2_3d * z3) / (z3 + z2);
                    q2 = (p3_3d * z1 + p1_3d * z3) / (z3 + z1);
                }
                else
                {
                    q1 = (p3_3d * z1 + p1_3d * z3) / (z3 + z1);
                    q2 = (p3_3d * z2 + p2_3d * z3) / (z3 + z2);

                }
            }

            res.renderLine(q1, q2);
        }
    }

}

LaserRangeFinder::RenderResult LaserRangeFinder::render(const Shape& shape, const Pose3D& cam_pose, const Pose3D& obj_pose, std::vector<double>& ranges) const
{

    LaserRangeFinder::RenderOptions options;
    options.setMesh(shape.getMesh(), cam_pose.inverse() * obj_pose);

    LaserRangeFinder::RenderResult res;

    res.ranges = ranges;
    render(options, res);
    ranges = res.ranges;

    return res;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//
//                                        PARAMETERS
//
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

void LaserRangeFinder::setAngleLimits(double min, double max) {
    a_min_ = min;
    a_max_ = max;
    if (num_beams_ > 0 && a_max_ - a_min_ > 0) {
        calculateRays();
    }
}

void LaserRangeFinder::setRangeLimits(double min, double max) {
    range_min_ = min;
    range_max_ = max;
}

void LaserRangeFinder::setNumBeams(int num_beams) {
    num_beams_ = num_beams;
    if (num_beams > 0 && a_max_ - a_min_ > 0) {
        calculateRays();
    }
}

void LaserRangeFinder::calculateRays() {
    ray_dirs_.clear();
    angles_.clear();

    double a_incr = getAngleIncrement();
    double a = a_min_;
    for(int i = 0; i < num_beams_; ++i) {
        Vector3 dir = polarTo2D(a, 1);
        ray_dirs_.push_back(dir);
        angles_.push_back(a);
        a += a_incr;
    }

    slope_factor_ = 1 / tan(getAngleIncrement()) * 10;
    int N = (int)slope_factor_ + 1;

    for(unsigned int i = 0; i < 8; ++i)
        slope_to_index_[i].resize(N);

    for(int j = 0; j < 8; ++j)
    {
        slope_to_index_[j].resize(N);
        for(int k = 0; k < N; ++k)
        {
            double slope = (double)k / slope_factor_;

            double x, y;
            if (j&1)
            {
                // x_abs > y_abs
                x = 1;
                y = slope + 1e-16; // Necessary because of 0-check in getAngle
            }
            else
            {
                // x_abs < y_abs
                x = slope + 1e-16; // Necessary because of 0-check in getAngle
                y = 1;
            }

            if (!(j&2))
            {
                // x < 0
                x = -x;
            }

            if (!(j&4))
            {
                // y < 0
                y = -y;
            }

            double a = getAngle(x, y);
            double a_temp = a - a_min_;
            if (a_temp < 0)
                a_temp += 2 * M_PI;

            slope_to_index_[j][k] = a_temp / (a_max_ - a_min_) * num_beams_ + 1;
        }
    }

    i_half_circle_ = M_PI / getAngleIncrement();
}

double LaserRangeFinder::getAngleMin() const {
    return a_min_;
}

double LaserRangeFinder::getAngleMax() const {
    return a_max_;
}

double LaserRangeFinder::getAngleIncrement() const {
    return (a_max_ - a_min_) / num_beams_;
}

const std::vector<double>& LaserRangeFinder::getAngles() const {
    return angles_;
}

int LaserRangeFinder::getAngleUpperIndex(double angle) const {
    int i = (angle - a_min_) / (a_max_ - a_min_) * num_beams_ + 1;
    return std::min(num_beams_, std::max(0, i));
}

int LaserRangeFinder::getAngleUpperIndex(double x, double y) const
{

    double x_abs = std::abs(x);
    double y_abs = std::abs(y);

    // Calculcate slope
    double slope;
    if (x_abs < y_abs)
        slope = x_abs / y_abs;
    else
        slope = y_abs / x_abs;

    int k = slope_factor_ * slope;

    // Calculate region number (0 - 7)
    int j = (x_abs < y_abs ? 0 : 1) + (x < 0 ? 0 : 2) + (y < 0 ? 0 : 4);

//    std::cout << "    j = " << j << ", k = " << k << std::endl;

//    std::cout << "    " << x << ", " << y << " --> " << slope_to_index_[j][k] << std::endl;

    return slope_to_index_[j][k];
}

double LaserRangeFinder::getRangeMin() const {
    return range_min_;
}

double LaserRangeFinder::getRangeMax() const {
    return range_max_;
}

int LaserRangeFinder::getNumBeams() const {
    return num_beams_;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
//
//                                        CONVERSIONS
//
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

geo::Vector3 LaserRangeFinder::rangeToPoint(double range, int i) const {
    return ray_dirs_[i] * range;
}

const geo::Vector3 LaserRangeFinder::getRayDirection(int i) const {
    return ray_dirs_[i];
}

bool LaserRangeFinder::rangesToPoints(const std::vector<double>& ranges, std::vector<geo::Vector3>& points) const {
    if (ranges.size() != ray_dirs_.size()) {
        return false;
    }
    points.resize(ray_dirs_.size());
    for(unsigned int i = 0; i < ray_dirs_.size(); ++i) {
        points[i] = ray_dirs_[i] * ranges[i];
    }
    return true;
}

geo::Vector3 LaserRangeFinder::polarTo2D(double angle, double range) {
    return geo::Vector3(cos(angle), sin(angle), 0) * range;
}

geo::Vector3 LaserRangeFinder::polarTo3D(const geo::Pose3D& laser_pose, double angle, double range) {
    return laser_pose.getBasis() * polarTo2D(angle, range);
}

double LaserRangeFinder::getAngle(double x, double y) {
    double a = atan(y / x);
//    double v = y / x;
//    double a = M_PI_4*v - v*(fabs(v) - 1)*(0.2447 + 0.0663*fabs(v));

    if (x < 0) {
        if (y < 0) {
            a = -M_PI + a;
        } else {
            a = M_PI + a;
        }
    }

    if (a > M_PI) {
        a -= 2 * M_PI ;
    } else if (a < -M_PI) {
        a += 2 * M_PI;
    }

    return a;
}

}
