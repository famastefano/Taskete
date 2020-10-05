#include <doctest.h>

#include "../source/taskete/execution_payload.hpp"

#include <memory>

// Callables used for this test
namespace
{
    // Just a helper to construct an execution_payload
    template<typename... Ts>
    std::unique_ptr<taskete::detail::execution_payload> create_payload(Ts&&... ts)
    {
        return std::make_unique<taskete::detail::UniversalCallable<Ts...>>(std::forward<Ts>(ts)...);
    }

    int non_capturing_lambda_result = 0;
    int const non_capturing_lambda_expected = 235986;

    int ff_result = 0;
    int const ff_expected = 1;

    int free_function() noexcept
    {
        ff_result = 1;
        return 0;
    }

    int const ffparam_expected = 96;

    void free_function_with_params(int& modify_me) noexcept
    {
        modify_me = 96;
    }

    class NonPolymorphicObject
    {
    private:
        int value = 0;

    public:
        void inc() noexcept { ++value; }
        int get() const noexcept { return value; }
    };

    class PolymorphicObjectBase
    {
    private:
        int value = 0;

    public:
        virtual void inc() noexcept { ++value; }
        int get() const noexcept { return value; }

        virtual ~PolymorphicObjectBase()
        {}
    };

    class PolymorphicObjectImplementation : virtual public PolymorphicObjectBase
    {
    public:
        // Just calls Base::inc() 2 times
        virtual void inc() noexcept { PolymorphicObjectBase::inc(); PolymorphicObjectBase::inc(); }
    };

    class InterfaceBase
    {
    public:
        virtual void process() noexcept = 0;
        virtual int read() const noexcept = 0;
        virtual ~InterfaceBase()
        {}
    };

    class ProcessByIncrement final : virtual public InterfaceBase
    {
    private:
        int value = 0;

    public:
        void process() noexcept override { ++value; }
        int read() const noexcept override { return value; }
    };
}

TEST_SUITE("Execution Payload")
{
    TEST_CASE("Calling a free function without parameters")
    {
        auto exec_payload = create_payload(free_function);
        
        (*exec_payload)();
        REQUIRE(ff_result == ff_expected);
    }

    TEST_CASE("Calling a free function with parameters")
    {
        int param = -124;
        auto exec_payload = create_payload(free_function_with_params, param);
        
        (*exec_payload)();
        REQUIRE(param == ffparam_expected);
    }

    TEST_CASE("Calling a non-capturing lambda")
    {
        auto exec_payload = create_payload([]
        {
            non_capturing_lambda_result = 235986;
        });

        (*exec_payload)();
        REQUIRE(non_capturing_lambda_result == non_capturing_lambda_expected);
    }

    TEST_CASE("Calling a capturing lambda")
    {
        int captured = 0;
        int expected = 23597;

        auto exec_payload = create_payload([&captured](int n)
        {
            captured = n;
        }, expected);
        
        (*exec_payload)();
        REQUIRE(captured == expected);
    }

    TEST_CASE("Calling a non-virtual member function")
    {
        NonPolymorphicObject obj{};

        auto exec_payload = create_payload(&NonPolymorphicObject::inc, obj);

        (*exec_payload)();
        REQUIRE(obj.get() == 1);
    }

    TEST_CASE("Calling a virtual member function")
    {
        PolymorphicObjectBase* base = new PolymorphicObjectImplementation;

        auto exec_payload = create_payload(&PolymorphicObjectBase::inc, base);

        (*exec_payload)();
        REQUIRE(base->get() == 2);

        delete base;
    }

    TEST_CASE("Calling a pure-virtual member function")
    {
        InterfaceBase* interface = new ProcessByIncrement;

        auto exec_payload = create_payload(&InterfaceBase::process, interface);

        (*exec_payload)();
        REQUIRE(interface->read() == 1);

        delete interface;
    }
}
