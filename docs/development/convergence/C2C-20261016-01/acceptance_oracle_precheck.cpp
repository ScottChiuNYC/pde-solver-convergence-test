#include "acceptance_reference.h"
#include <iomanip>
#include <iostream>

int main() {
    using namespace acceptance_reference;
    const auto known=denseSolve({{4,1,0},{1,4,1},{0,1,4}},{6,12,14});
    for (std::size_t i=0;i<3;++i)
        if (std::abs(known[i]-double(i+1))>1e-15) return 1;
    Data row{
        [](double t,double) { return 1+t; },
        [](double t,double) { return 2*t; },
        [](double t,double) { return -2*t; },
        [](double t,double) { return 2+4*t; },
        [](double,double) { return 2.0; },
        [](double t,double) { return 1+2*t; },
        [](double t,double) { return 5-t; }
    };
    std::cout<<std::setprecision(17);
    for (double theta : {0.5,0.75,1.0}) {
        const auto result=evolve(row,0,2,0,1,3,1,theta);
        const double expected=(7+12*theta)/(1+4*theta);
        if (result[0]!=3||result[2]!=4||std::abs(result[1]-expected)>1e-15) return 2;
        std::cout<<"one_row theta="<<theta<<" interior="<<result[1]<<" analytic="<<expected<<'\n';
    }
    const auto norm=errors({0,1,0},0,1,0,[](double,double) { return 0.0; });
    if (norm.infinity!=1||norm.l2!=std::sqrt(0.5)) return 3;
    if (endpointMean(std::numeric_limits<double>::max(),std::numeric_limits<double>::max())
        !=std::numeric_limits<double>::max()) return 4;
    for (double theta : {0.5,0.75,1.0}) {
        const auto result=evolve(mixedData(),-0.5,1.3,-0.2,0.3,7,4,theta);
        std::cout<<"mixed_dense theta="<<theta<<" values=";
        for (double value : result) std::cout<<value<<',';
        std::cout<<'\n';
    }
    std::cout<<"PASS: oracle has no production dependency; known dense solve, exact row, norm, safe mean checked\n";
}
