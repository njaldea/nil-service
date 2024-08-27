#pragma once

namespace nil::service::detail
{
    /**
     * @brief used to cause compilation error with customizable error message
     *
     * @tparam T
     */
    template <typename T, typename... Rest>
    void error(Rest...) = delete;

    /**
     * @brief used to cause compilation error in case when there is a missing branch
     *
     * @tparam T
     */
    template <typename T>
    void unreachable() = delete;
}
