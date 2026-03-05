#include "backend.hpp"

#include "aos_backend.hpp"
#include "soa_backend.hpp"

std::unique_ptr<Backend> create_backend(AntLayout layout,
                                        std::vector<ant>& ants_aos,
                                        AntsSoA& ants_soa)
{
    if (layout == AntLayout::aos) {
        return std::make_unique<AosBackend>(ants_aos);
    }
    return std::make_unique<SoaBackend>(ants_soa);
}
