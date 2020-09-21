#pragma once

#include <tuple>

namespace taskete::detail
{
    /// <summary>
    /// Wrapper to call any kind of callable that std::invoke handles.
    /// </summary>
    class ExecutionPayload
    {
    public:
        virtual void operator()() noexcept = 0;

        virtual ~ExecutionPayload()
        {}
    };

    template<typename Callable, typename... Args>
    class UniversalCallable final : public virtual ExecutionPayload
    {
    private:
        Callable c;
        std::tuple<Args...> params;

    public:
        UniversalCallable(Callable&& c, Args&&... args) : c(std::forward<Callable>(c)), params(std::forward<Args>(args)...)
        {}

        void operator()() noexcept override
        {
            std::apply(c, params);
        }
    };
}