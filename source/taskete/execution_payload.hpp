#pragma once

#include <tuple>

namespace taskete::detail
{
    /// <summary>
    /// Wrapper to call any kind of callable that std::invoke handles.
    /// </summary>
    class execution_payload
    {
    public:
        virtual void operator()() noexcept = 0;

        virtual std::size_t size_of() const noexcept = 0;
        
        virtual ~execution_payload() // we need the destructor to cleanup non-trivial objects used as parameters
        {}
    };
    
    template<typename Callable, typename... Args>
    class universal_callable final : public virtual execution_payload
    {
    private:
        Callable c;
        std::tuple<Args...> params;

    public:
        universal_callable(Callable&& c, Args&&... args) : c(std::forward<Callable>(c)), params(std::forward<Args>(args)...)
        {}

        void operator()() noexcept override
        {
            std::apply(c, params);
        }

        std::size_t size_of() const noexcept override
        {
            return sizeof(*this);
        }
    };
}