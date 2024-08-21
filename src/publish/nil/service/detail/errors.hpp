#pragma once

namespace nil::service::detail
{
    /**
     * @brief used to cause compilation error when argument types of a callable is invalid
     *
     * @tparam T
     */
    template <typename T, typename... Rest>
    void argument_error(Rest...) = delete;

    /**
     * @brief used to cause compilation error in case when there is a missing branch
     *
     * @tparam T
     */
    template <typename T>
    void unreachable() = delete;
}
