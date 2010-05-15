/*
   Copyright (C) 2009 Anton Mihalyov <anton@glyphsense.com>

   This  library is  free software;  you can  redistribute it  and/or
   modify  it under  the  terms  of the  GNU  Library General  Public
   License  (LGPL)  as published  by  the  Free Software  Foundation;
   either version  2 of the  License, or  (at your option)  any later
   version.

   This library  is distributed in the  hope that it will  be useful,
   but WITHOUT  ANY WARRANTY;  without even  the implied  warranty of
   MERCHANTABILITY or FITNESS  FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy  of the GNU Library General Public
   License along with this library; see the file COPYING.LIB. If not,
   write to the  Free Software Foundation, Inc.,  51 Franklin Street,
   Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef DELEGATE_BIND_HH_
#define DELEGATE_BIND_HH_

#include <memory>

namespace Hypergrace {
namespace Delegate {

namespace Details {
    template<typename ResultType, typename Class, typename... Args>
    class MemberFnWrapper
    {
    public:
        MemberFnWrapper(ResultType (Class::*f)(Args...)) : function_(f) {}

        template<typename This>
        inline ResultType operator ()(This *this_, const Args &... args) const
        {
            // TODO: write a proper explanation why constness is
            // modified here
            return (this_->*function_)(const_cast<Args &>(args)...);
        }

        template<typename This>
        inline ResultType operator ()(const std::shared_ptr<This> &this_,
                                      const Args &... args) const
        {
            return (const_cast<This *>(this_.get())->*function_)(const_cast<Args &>(args)...);
        }

    private:
        ResultType (Class::*function_)(Args...);
    };

    template<typename ResultType, typename Class, typename... Args>
    class ConstMemberFnWrapper
    {
    public:
        ConstMemberFnWrapper(ResultType (Class::*f)(Args...) const) : function_(f) {}

        template<typename This>
        inline ResultType operator ()(const This *this_, const Args &... args) const
        {
            return (this_->*function_)(const_cast<Args &>(args)...);
        }

        template<typename This>
        inline ResultType operator ()(const std::shared_ptr<This> &this_,
                                      const Args &... args) const
        {
            return (this_.get()->*function_)(const_cast<Args &>(args)...);
        }

    private:
        ResultType (Class::*function_)(Args...) const;
    };

    template<typename ResultType, typename Class, typename... Args>
    inline MemberFnWrapper<ResultType, Class, Args...>
            invokable(ResultType (Class::*function)(Args...))
    {
        return MemberFnWrapper<ResultType, Class, Args...>(function);
    }

    template<typename ResultType, typename Class, typename... Args>
    inline ConstMemberFnWrapper<ResultType, Class, Args...>
            invokable(ResultType (Class::*function)(Args...) const)
    {
        return ConstMemberFnWrapper<ResultType, Class, Args...>(function);
    }

    template<typename Function>
    inline Function invokable(Function &f)
    {
        return f;
    }
}

template<int n> struct Placeholder {};

class ArgumentPack0
{
public:
    template<typename T> inline T &operator [](T &t) const { return t; }
    template<typename T> inline const T &operator [](const T &t) const { return t; }

    template<typename ResultType, typename Function, typename RealArgPack>
    inline ResultType invoke(Function &f, RealArgPack &ap)
    {
        return Details::invokable(f)();
    }
};

template<typename Arg1>
class ArgumentPack1
{
public:
    ArgumentPack1(const Arg1 &a1) :
        arg1_(a1)
    {
    }

    inline Arg1 &operator [](const Placeholder<1> &) { return arg1_; }
    //inline const Arg1 &operator [](const Placeholder<1> &) const { return arg1_; }

    template<typename T> inline T &operator [](T &t) const { return t; }
    template<typename T> inline const T &operator [](const T &t) const { return t; }

    template<typename ResultType, typename Function, typename RealArgPack>
    inline ResultType invoke(Function &f, RealArgPack &ap)
    {
        return Details::invokable(f)(ap[arg1_]);
    }

protected:
    Arg1 arg1_;
};

template<typename Arg1, typename Arg2>
class ArgumentPack2
{
public:
    ArgumentPack2(const Arg1 &a1, const Arg2 &a2) :
        arg1_(a1), arg2_(a2)
    {
    }

    inline Arg1 &operator [](const Placeholder<1> &) { return arg1_; }
    inline Arg2 &operator [](const Placeholder<2> &) { return arg2_; }

    //inline const Arg1 &operator [](const Placeholder<1> &) const { return arg1_; }
    //inline const Arg2 &operator [](const Placeholder<2> &) const { return arg2_; }

    template<typename T> inline T &operator [](T &t) const { return t; }
    template<typename T> inline const T &operator [](const T &t) const { return t; }

    template<typename ResultType, typename Function, typename RealArgPack>
    inline ResultType invoke(Function &f, RealArgPack &ap)
    {
        return Details::invokable(f)(ap[arg1_], ap[arg2_]);
    }

protected:
    Arg1 arg1_; Arg2 arg2_;
};

template<typename Arg1, typename Arg2, typename Arg3>
class ArgumentPack3
{
public:
    ArgumentPack3(const Arg1 &a1, const Arg2 &a2, const Arg3 &a3) :
        arg1_(a1), arg2_(a2), arg3_(a3)
    {
    }

    inline Arg1 &operator [](const Placeholder<1> &) { return arg1_; }
    inline Arg2 &operator [](const Placeholder<2> &) { return arg2_; }
    inline Arg3 &operator [](const Placeholder<3> &) { return arg3_; }

    //inline const Arg1 &operator [](const Placeholder<1> &) const { return arg1_; }
    //inline const Arg2 &operator [](const Placeholder<2> &) const { return arg2_; }
    //inline const Arg3 &operator [](const Placeholder<3> &) const { return arg3_; }

    template<typename T> inline T &operator [](T &t) const { return t; }
    template<typename T> inline const T &operator [](const T &t) const { return t; }

    template<typename ResultType, typename Function, typename RealArgPack>
    inline ResultType invoke(Function &f, RealArgPack &ap)
    {
        return Details::invokable(f)(ap[arg1_], ap[arg2_], ap[arg3_]);
    }

protected:
    Arg1 arg1_; Arg2 arg2_; Arg3 arg3_;
};

template<typename Arg1, typename Arg2, typename Arg3, typename Arg4>
class ArgumentPack4
{
public:
    ArgumentPack4(const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4) :
        arg1_(a1), arg2_(a2), arg3_(a3), arg4_(a4)
    {
    }

    inline Arg1 &operator [](const Placeholder<1> &) { return arg1_; }
    inline Arg2 &operator [](const Placeholder<2> &) { return arg2_; }
    inline Arg3 &operator [](const Placeholder<3> &) { return arg3_; }
    inline Arg4 &operator [](const Placeholder<4> &) { return arg4_; }

    //inline const Arg1 &operator [](const Placeholder<1> &) const { return arg1_; }
    //inline const Arg2 &operator [](const Placeholder<2> &) const { return arg2_; }
    //inline const Arg3 &operator [](const Placeholder<3> &) const { return arg3_; }
    //inline const Arg4 &operator [](const Placeholder<4> &) const { return arg4_; }

    template<typename T> inline T &operator [](T &t) const { return t; }
    template<typename T> inline const T &operator [](const T &t) const { return t; }

    template<typename ResultType, typename Function, typename RealArgPack>
    inline ResultType invoke(Function &f, RealArgPack &ap)
    {
        return Details::invokable(f)(ap[arg1_], ap[arg2_], ap[arg3_], ap[arg4_]);
    }

protected:
    Arg1 arg1_; Arg2 arg2_; Arg3 arg3_; Arg4 arg4_;
};

template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
class ArgumentPack5
{
public:
    ArgumentPack5(const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4, const Arg5 &a5) :
        arg1_(a1), arg2_(a2), arg3_(a3), arg4_(a4), arg5_(a5)
    {
    }

    inline Arg1 &operator [](const Placeholder<1> &) { return arg1_; }
    inline Arg2 &operator [](const Placeholder<2> &) { return arg2_; }
    inline Arg3 &operator [](const Placeholder<3> &) { return arg3_; }
    inline Arg4 &operator [](const Placeholder<4> &) { return arg4_; }
    inline Arg5 &operator [](const Placeholder<5> &) { return arg5_; }

    //inline const Arg1 &operator [](const Placeholder<1> &) const { return arg1_; }
    //inline const Arg2 &operator [](const Placeholder<2> &) const { return arg2_; }
    //inline const Arg3 &operator [](const Placeholder<3> &) const { return arg3_; }
    //inline const Arg4 &operator [](const Placeholder<4> &) const { return arg4_; }
    //inline const Arg5 &operator [](const Placeholder<5> &) const { return arg5_; }

    template<typename T> inline T &operator [](T &t) const { return t; }
    template<typename T> inline const T &operator [](const T &t) const { return t; }

    template<typename ResultType, typename Function, typename RealArgPack>
    inline ResultType invoke(Function &f, RealArgPack &ap)
    {
        return Details::invokable(f)(ap[arg1_], ap[arg2_], ap[arg3_], ap[arg4_], ap[arg5_]);
    }

protected:
    Arg1 arg1_; Arg2 arg2_; Arg3 arg3_; Arg4 arg4_; Arg5 arg5_;
};

template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
class ArgumentPack6
{
public:
    ArgumentPack6(const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4, const Arg5 &a5,
                  const Arg6 &a6) :
        arg1_(a1), arg2_(a2), arg3_(a3), arg4_(a4), arg5_(a5), arg6_(a6)
    {
    }

    inline Arg1 &operator [](const Placeholder<1> &) { return arg1_; }
    inline Arg2 &operator [](const Placeholder<2> &) { return arg2_; }
    inline Arg3 &operator [](const Placeholder<3> &) { return arg3_; }
    inline Arg4 &operator [](const Placeholder<4> &) { return arg4_; }
    inline Arg5 &operator [](const Placeholder<5> &) { return arg5_; }
    inline Arg6 &operator [](const Placeholder<6> &) { return arg6_; }

    //inline const Arg1 &operator [](const Placeholder<1> &) const { return arg1_; }
    //inline const Arg2 &operator [](const Placeholder<2> &) const { return arg2_; }
    //inline const Arg3 &operator [](const Placeholder<3> &) const { return arg3_; }
    //inline const Arg4 &operator [](const Placeholder<4> &) const { return arg4_; }
    //inline const Arg5 &operator [](const Placeholder<5> &) const { return arg5_; }
    //inline const Arg6 &operator [](const Placeholder<6> &) const { return arg6_; }

    template<typename T> inline T &operator [](T &t) const { return t; }
    template<typename T> inline const T &operator [](const T &t) const { return t; }

    template<typename ResultType, typename Function, typename RealArgPack>
    inline ResultType invoke(Function &f, RealArgPack &ap)
    {
        return Details::invokable(f)(ap[arg1_], ap[arg2_], ap[arg3_], ap[arg4_], ap[arg5_],
                                     ap[arg6_]);
    }

protected:
    Arg1 arg1_; Arg2 arg2_; Arg3 arg3_; Arg4 arg4_; Arg5 arg5_; Arg6 arg6_;
};

template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6,
         typename Arg7>
class ArgumentPack7
{
public:
    ArgumentPack7(const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4, const Arg5 &a5,
                  const Arg6 &a6, const Arg7 &a7) :
        arg1_(a1), arg2_(a2), arg3_(a3), arg4_(a4), arg5_(a5), arg6_(a6), arg7_(a7)
    {
    }

    inline Arg1 &operator [](const Placeholder<1> &) { return arg1_; }
    inline Arg2 &operator [](const Placeholder<2> &) { return arg2_; }
    inline Arg3 &operator [](const Placeholder<3> &) { return arg3_; }
    inline Arg4 &operator [](const Placeholder<4> &) { return arg4_; }
    inline Arg5 &operator [](const Placeholder<5> &) { return arg5_; }
    inline Arg6 &operator [](const Placeholder<6> &) { return arg6_; }
    inline Arg6 &operator [](const Placeholder<7> &) { return arg7_; }

    //inline const Arg1 &operator [](const Placeholder<1> &) const { return arg1_; }
    //inline const Arg2 &operator [](const Placeholder<2> &) const { return arg2_; }
    //inline const Arg3 &operator [](const Placeholder<3> &) const { return arg3_; }
    //inline const Arg4 &operator [](const Placeholder<4> &) const { return arg4_; }
    //inline const Arg5 &operator [](const Placeholder<5> &) const { return arg5_; }
    //inline const Arg6 &operator [](const Placeholder<6> &) const { return arg6_; }
    //inline const Arg6 &operator [](const Placeholder<7> &) const { return arg7_; }

    template<typename T> inline T &operator [](T &t) const { return t; }
    template<typename T> inline const T &operator [](const T &t) const { return t; }

    template<typename ResultType, typename Function, typename RealArgPack>
    inline ResultType invoke(Function &f, RealArgPack &ap)
    {
        return Details::invokable(f)(ap[arg1_], ap[arg2_], ap[arg3_], ap[arg4_], ap[arg5_],
                                     ap[arg6_], ap[arg7_]);
    }

protected:
    Arg1 arg1_; Arg2 arg2_; Arg3 arg3_; Arg4 arg4_; Arg5 arg5_; Arg6 arg6_; Arg7 arg7_;
};

template<typename ResultType, typename Function, typename Pack>
class Binder
{
public:
    Binder(const Function &f, const Pack &p) :
        func_(f), pack_(p) {}

    inline ResultType operator ()()
    {
        typedef ArgumentPack0 RealArgs;
        RealArgs args;

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1>
    inline ResultType operator ()(Arg1 &a1)
    {
        typedef ArgumentPack1<Arg1 &> RealArgs;
        RealArgs args(a1);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1>
    inline ResultType operator ()(const Arg1 &a1)
    {
        typedef ArgumentPack1<const Arg1 &> RealArgs;
        RealArgs args(a1);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2>
    inline ResultType operator ()(Arg1 &a1, Arg2 &a2)
    {
        typedef ArgumentPack2<Arg1 &, Arg2 &> RealArgs;
        RealArgs args(a1, a2);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2>
    inline ResultType operator ()(const Arg1 &a1, const Arg2 &a2)
    {
        typedef ArgumentPack2<const Arg1 &, const Arg2 &> RealArgs;
        RealArgs args(a1, a2);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3>
    inline ResultType operator ()(Arg1 &a1, Arg2 &a2, Arg3 &a3)
    {
        typedef ArgumentPack3<Arg1 &, Arg2 &, Arg3 &> RealArgs;
        RealArgs args(a1, a2, a3);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3>
    inline ResultType operator ()(const Arg1 &a1, const Arg2 &a2, const Arg3 &a3)
    {
        typedef ArgumentPack3<const Arg1 &, const Arg2 &, const Arg3 &> RealArgs;
        RealArgs args(a1, a2, a3);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    inline ResultType operator ()(Arg1 &a1, Arg2 &a2, Arg3 &a3, Arg4 &a4)
    {
        typedef ArgumentPack4<Arg1 &, Arg2 &, Arg3 &, Arg4 &> RealArgs;
        RealArgs args(a1, a2, a3, a4);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    inline ResultType operator ()(const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4)
    {
        typedef ArgumentPack4<const Arg1 &, const Arg2 &, const Arg3 &, const Arg4 &> RealArgs;
        RealArgs args(a1, a2, a3, a4);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    inline ResultType operator ()(Arg1 &a1, Arg2 &a2, Arg3 &a3, Arg4 &a4, Arg5 &a5)
    {
        typedef ArgumentPack5<Arg1 &, Arg2 &, Arg3 &, Arg4 &, Arg5 &> RealArgs;
        RealArgs args(a1, a2, a3, a4, a5);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    inline ResultType
        operator ()(const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4, \
                    const Arg5 &a5)
    {
        typedef ArgumentPack5<const Arg1 &, const Arg2 &, const Arg3 &, const Arg4 &,
                              const Arg5 &> RealArgs;
        RealArgs args(a1, a2, a3, a4, a5);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
             typename Arg6>
    inline ResultType
        operator ()(Arg1 &a1, Arg2 &a2, Arg3 &a3, Arg4 &a4, Arg5 &a5, Arg6 &a6)
    {
        typedef ArgumentPack6<Arg1 &, Arg2 &, Arg3 &, Arg4 &, Arg5 &, Arg6 &> RealArgs;
        RealArgs args(a1, a2, a3, a4, a5, a6);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, \
             typename Arg6>
    inline ResultType
        operator ()(const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4, 
                    const Arg5 &a5, const Arg6 &a6)
    {
        typedef ArgumentPack6<const Arg1 &, const Arg2 &, const Arg3 &, const Arg4 &,
                              const Arg5 &, const Arg6 &> RealArgs;
        RealArgs args(a1, a2, a3, a4, a5, a6);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
             typename Arg6, typename Arg7>
    inline ResultType
        operator ()(Arg1 &a1, Arg2 &a2, Arg3 &a3, Arg4 &a4, Arg5 &a5, Arg6 &a6, Arg7 &a7)
    {
        typedef ArgumentPack7<Arg1 &, Arg2 &, Arg3 &, Arg4 &, Arg5 &, Arg6 &, Arg7 &> RealArgs;
        RealArgs args(a1, a2, a3, a4, a5, a6, a7);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, \
             typename Arg6, typename Arg7>
    inline ResultType
        operator ()(const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4, 
                    const Arg5 &a5, const Arg6 &a6, const Arg7 &a7)
    {
        typedef ArgumentPack7<const Arg1 &, const Arg2 &, const Arg3 &, const Arg4 &,
                              const Arg5 &, const Arg6 &, const Arg7 &> RealArgs;
        RealArgs args(a1, a2, a3, a4, a5, a6, a7);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    ResultType invoke()
    {
        typedef ArgumentPack0 RealArgs;
        RealArgs args;

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1>
    ResultType invoke(Arg1 a1)
    {
        typedef ArgumentPack1<Arg1 &> RealArgs;
        RealArgs args(a1);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2>
    ResultType invoke(Arg1 a1, Arg2 a2)
    {
        typedef ArgumentPack2<Arg1 &, Arg2 &> RealArgs;
        RealArgs args(a1, a2);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3>
    ResultType invoke(Arg1 a1, Arg2 a2, Arg3 a3)
    {
        typedef ArgumentPack3<Arg1 &, Arg2 &, Arg3 &> RealArgs;
        RealArgs args(a1, a2, a3);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    ResultType invoke(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4)
    {
        typedef ArgumentPack4<Arg1 &, Arg2 &, Arg3 &, Arg4 &> RealArgs;
        RealArgs args(a1, a2, a3, a4);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    ResultType invoke(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5)
    {
        typedef ArgumentPack5<Arg1 &, Arg2 &, Arg3 &, Arg4 &, Arg5 &> RealArgs;
        RealArgs args(a1, a2, a3, a4, a5);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
             typename Arg6>
    ResultType invoke(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5, Arg6 a6)
    {
        typedef ArgumentPack6<Arg1 &, Arg2 &, Arg3 &, Arg4 &, Arg5 &, Arg6 &> RealArgs;
        RealArgs args(a1, a2, a3, a4, a5, a6);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

    template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
             typename Arg6, typename Arg7>
    ResultType invoke(Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, Arg5 a5, Arg6 a6, Arg7 a7)
    {
        typedef ArgumentPack7<Arg1 &, Arg2 &, Arg3 &, Arg4 &, Arg5 &, Arg6 &, Arg7 &> RealArgs;
        RealArgs args(a1, a2, a3, a4, a5, a6, a7);

        return pack_.template invoke<ResultType, Function, RealArgs>(func_, args);
    }

private:
    Function func_;
    Pack pack_;
};

/* Member functions */

template<typename R, typename C, typename This,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack1<This> >
    bind(R (C::*f)(Sig...), const This &this_)
{
    typedef ArgumentPack1<This> Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_));
}

template<typename R, typename C, typename This,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack1<This> >
    bind(R (C::*f)(Sig...) const, const This &this_)
{
    typedef ArgumentPack1<This> Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_));
}

template<typename R, typename C, typename This,
         typename Arg1,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack2<This, Arg1> >
    bind(R (C::*f)(Sig...), const This &this_, const Arg1 &a1)
{
    typedef ArgumentPack2<This, Arg1> Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_, a1));
}

template<typename R, typename C, typename This,
         typename Arg1,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack2<This, Arg1> >
    bind(R (C::*f)(Sig...) const, const This &this_, const Arg1 &a1)
{
    typedef ArgumentPack2<This, Arg1> Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_, a1));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack3<This, Arg1, Arg2> >
    bind(R (C::*f)(Sig...), const This &this_, const Arg1 &a1, const Arg2 &a2)
{
    typedef ArgumentPack3<This, Arg1, Arg2> Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_, a1, a2));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack3<This, Arg1, Arg2> >
    bind(R (C::*f)(Sig...) const, const This &this_, const Arg1 &a1, const Arg2 &a2)
{
    typedef ArgumentPack3<This, Arg1, Arg2> Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_, a1, a2));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack4<This, Arg1, Arg2, Arg3> >
    bind(R (C::*f)(Sig...), const This &this_, const Arg1 &a1, const Arg2 &a2, const Arg3 &a3)
{
    typedef ArgumentPack4<This, Arg1, Arg2, Arg3> Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_, a1, a2, a3));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack4<This, Arg1, Arg2, Arg3> >
    bind(R (C::*f)(Sig...) const, const This &this_, const Arg1 &a1, const Arg2 &a2,
                                  const Arg3 &a3)
{
    typedef ArgumentPack4<This, Arg1, Arg2, Arg3> Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_, a1, a2, a3));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3, typename Arg4,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack5<This, Arg1, Arg2, Arg3, Arg4> >
    bind(R (C::*f)(Sig...), const This &this_, const Arg1 &a1, const Arg2 &a2, const Arg3 &a3,
                            const Arg4 &a4)
{
    typedef ArgumentPack5<This, Arg1, Arg2, Arg3, Arg4> Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_, a1, a2, a3, a4));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3, typename Arg4,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack5<This, Arg1, Arg2, Arg3, Arg4> >
    bind(R (C::*f)(Sig...) const, const This &this_, const Arg1 &a1, const Arg2 &a2,
                           const Arg3 &a3, const Arg4 &a4)
{
    typedef ArgumentPack5<This, Arg1, Arg2, Arg3, Arg4> Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_, a1, a2, a3, a4));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack6<This, Arg1, Arg2, Arg3, Arg4, Arg5> >
    bind(R (C::*f)(Sig...), const This &this_, const Arg1 &a1, const Arg2 &a2, const Arg3 &a3,
                            const Arg4 &a4, const Arg5 &a5)
{
    typedef ArgumentPack6<This, Arg1, Arg2, Arg3, Arg4, Arg5> Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_, a1, a2, a3, a4, a5));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack6<This, Arg1, Arg2, Arg3, Arg4, Arg5> >
    bind(R (C::*f)(Sig...) const, const This &this_, const Arg1 &a1, const Arg2 &a2,
                           const Arg3 &a3, const Arg4 &a4, const Arg5 &a5)
{
    typedef ArgumentPack6<This, Arg1, Arg2, Arg3, Arg4, Arg5> Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_, a1, a2, a3, a4, a5));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack7<This, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6> >
    bind(R (C::*f)(Sig...), const This &this_, const Arg1 &a1, const Arg2 &a2, const Arg3 &a3,
                            const Arg4 &a4, const Arg5 &a5, const Arg6 &a6)
{
    typedef ArgumentPack7<This, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6> Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_, a1, a2, a3, a4, a5, a6));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack7<This, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6> >
    bind(R (C::*f)(Sig...) const, const This &this_, const Arg1 &a1, const Arg2 &a2,
                           const Arg3 &a3, const Arg4 &a4, const Arg5 &a5, const Arg6 &a6)
{
    typedef ArgumentPack7<This, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6> Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_, a1, a2, a3, a4, a5, a6));
}

/* Member functions with shared_ptr self pointer */

template<typename R, typename C, typename This,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack1<std::shared_ptr<This> > >
    bind(R (C::*f)(Sig...), const std::shared_ptr<This> &this_)
{
    typedef ArgumentPack1<std::shared_ptr<This> > Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_));
}

template<typename R, typename C, typename This,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack1<std::shared_ptr<This> > >
    bind(R (C::*f)(Sig...) const, const std::shared_ptr<This> &this_)
{
    typedef ArgumentPack1<std::shared_ptr<This> > Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_));
}

template<typename R, typename C, typename This,
         typename Arg1,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack2<std::shared_ptr<This>, Arg1> >
    bind(R (C::*f)(Sig...), const std::shared_ptr<This> &this_, const Arg1 &a1)
{
    typedef ArgumentPack2<std::shared_ptr<This>, Arg1> Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_, a1));
}

template<typename R, typename C, typename This,
         typename Arg1,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack2<std::shared_ptr<This>, Arg1> >
    bind(R (C::*f)(Sig...) const, const std::shared_ptr<This> &this_, const Arg1 &a1)
{
    typedef ArgumentPack2<std::shared_ptr<This>, Arg1> Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_, a1));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack3<std::shared_ptr<This>, Arg1, Arg2> >
    bind(R (C::*f)(Sig...), const std::shared_ptr<This> &this_, const Arg1 &a1, const Arg2 &a2)
{
    typedef ArgumentPack3<std::shared_ptr<This>, Arg1, Arg2> Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_, a1, a2));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack3<std::shared_ptr<This>, Arg1, Arg2> >
    bind(R (C::*f)(Sig...) const, const std::shared_ptr<This> &this_, const Arg1 &a1,
                                  const Arg2 &a2)
{
    typedef ArgumentPack3<std::shared_ptr<This>, Arg1, Arg2> Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_, a1, a2));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack4<std::shared_ptr<This>, Arg1, Arg2, Arg3> >
    bind(R (C::*f)(Sig...), const std::shared_ptr<This> &this_, const Arg1 &a1, const Arg2 &a2,
                            const Arg3 &a3)
{
    typedef ArgumentPack4<std::shared_ptr<This>, Arg1, Arg2, Arg3> Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_, a1, a2, a3));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack4<std::shared_ptr<This>, Arg1, Arg2, Arg3> >
    bind(R (C::*f)(Sig...) const, const std::shared_ptr<This> &this_, const Arg1 &a1,
                                  const Arg2 &a2, const Arg3 &a3)
{
    typedef ArgumentPack4<std::shared_ptr<This>, Arg1, Arg2, Arg3> Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_, a1, a2, a3));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3, typename Arg4,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack5<std::shared_ptr<This>, Arg1, Arg2, Arg3, Arg4> >
    bind(R (C::*f)(Sig...), const std::shared_ptr<This> &this_, const Arg1 &a1, const Arg2 &a2,
                            const Arg3 &a3, const Arg4 &a4)
{
    typedef ArgumentPack5<std::shared_ptr<This>, Arg1, Arg2, Arg3, Arg4> Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_, a1, a2, a3, a4));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3, typename Arg4,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack5<std::shared_ptr<This>, Arg1, Arg2, Arg3, Arg4> >
    bind(R (C::*f)(Sig...) const, const std::shared_ptr<This> &this_, const Arg1 &a1,
                                  const Arg2 &a2, const Arg3 &a3, const Arg4 &a4)
{
    typedef ArgumentPack5<std::shared_ptr<This>, Arg1, Arg2, Arg3, Arg4> Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_, a1, a2, a3, a4));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack6<std::shared_ptr<This>, Arg1, Arg2, Arg3, Arg4, Arg5> >
    bind(R (C::*f)(Sig...), const std::shared_ptr<This> &this_, const Arg1 &a1, const Arg2 &a2,
                            const Arg3 &a3, const Arg4 &a4, const Arg5 &a5)
{
    typedef ArgumentPack6<std::shared_ptr<This>, Arg1, Arg2, Arg3, Arg4, Arg5> Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_, a1, a2, a3, a4, a5));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack6<std::shared_ptr<This>, Arg1, Arg2, Arg3, Arg4,
                                                Arg5> >
    bind(R (C::*f)(Sig...) const, const std::shared_ptr<This> &this_, const Arg1 &a1,
                                  const Arg2 &a2, const Arg3 &a3, const Arg4 &a4, const Arg5 &a5)
{
    typedef ArgumentPack6<std::shared_ptr<This>, Arg1, Arg2, Arg3, Arg4, Arg5> Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_, a1, a2, a3, a4, a5));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6,
         typename... Sig>
Binder<R, R (C::*)(Sig...), ArgumentPack7<std::shared_ptr<This>, Arg1, Arg2, Arg3, Arg4, Arg5,
                                          Arg6> >
    bind(R (C::*f)(Sig...), const std::shared_ptr<This> &this_, const Arg1 &a1, const Arg2 &a2,
                            const Arg3 &a3, const Arg4 &a4, const Arg5 &a5, const Arg6 &a6)
{
    typedef ArgumentPack7<std::shared_ptr<This>, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6> Pack;
    return Binder<R, R (C::*)(Sig...), Pack>(f, Pack(this_, a1, a2, a3, a4, a5, a6));
}

template<typename R, typename C, typename This,
         typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6,
         typename... Sig>
Binder<R, R (C::*)(Sig...) const, ArgumentPack7<std::shared_ptr<This>, Arg1, Arg2, Arg3, Arg4,
                                                Arg5, Arg6> >
    bind(R (C::*f)(Sig...) const, const std::shared_ptr<This> &this_, const Arg1 &a1,
                                  const Arg2 &a2, const Arg3 &a3, const Arg4 &a4, const Arg5 &a5,
                                  const Arg6 &a6)
{
    typedef ArgumentPack7<std::shared_ptr<This>, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6> Pack;
    return Binder<R, R (C::*)(Sig...) const, Pack>(f, Pack(this_, a1, a2, a3, a4, a5, a6));
}

/* Non-member functions & static functions */

template<typename R,
         typename... Sig>
Binder<R, R (*)(Sig...), ArgumentPack0 >
    bind(R (*f)(Sig...))
{
    typedef ArgumentPack0 Pack;
    return Binder<R, R (*)(Sig...), Pack>(f, Pack());
}

template<typename R, typename Arg1,
         typename... Sig>
Binder<R, R (*)(Sig...), ArgumentPack1<Arg1> >
    bind(R (*f)(Sig...), const Arg1 &a1)
{
    typedef ArgumentPack1<Arg1> Pack;
    return Binder<R, R (*)(Sig...), Pack>(f, Pack(a1));
}

template<typename R, typename Arg1, typename Arg2,
         typename... Sig>
Binder<R, R (*)(Sig...), ArgumentPack2<Arg1, Arg2> >
    bind(R (*f)(Sig...), const Arg1 &a1, const Arg2 &a2)
{
    typedef ArgumentPack2<Arg1, Arg2> Pack;
    return Binder<R, R (*)(Sig...), Pack>(f, Pack(a1, a2));
}

template<typename R, typename Arg1, typename Arg2, typename Arg3,
         typename... Sig>
Binder<R, R (*)(Sig...), ArgumentPack3<Arg1, Arg2, Arg3> >
    bind(R (*f)(Sig...), const Arg1 &a1, const Arg2 &a2, const Arg3 &a3)
{
    typedef ArgumentPack3<Arg1, Arg2, Arg3> Pack;
    return Binder<R, R (*)(Sig...), Pack>(f, Pack(a1, a2, a3));
}

template<typename R, typename Arg1, typename Arg2, typename Arg3, typename Arg4,
         typename... Sig>
Binder<R, R (*)(Sig...), ArgumentPack4<Arg1, Arg2, Arg3, Arg4> >
    bind(R (*f)(Sig...), const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4)
{
    typedef ArgumentPack4<Arg1, Arg2, Arg3, Arg4> Pack;
    return Binder<R, R (*)(Sig...), Pack>(f, Pack(a1, a2, a3, a4));
}

template<typename R, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
         typename... Sig>
Binder<R, R (*)(Sig...), ArgumentPack5<Arg1, Arg2, Arg3, Arg4, Arg5> >
    bind(R (*f)(Sig...), const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4,
                         const Arg5 &a5)
{
    typedef ArgumentPack5<Arg1, Arg2, Arg3, Arg4, Arg5> Pack;
    return Binder<R, R (*)(Sig...), Pack>(f, Pack(a1, a2, a3, a4, a5));
}

template<typename R, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
                     typename Arg6,
         typename... Sig>
Binder<R, R (*)(Sig...), ArgumentPack6<Arg1, Arg2, Arg3, Arg4, Arg5, Arg6> >
    bind(R (*f)(Sig...), const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4,
                         const Arg5 &a5, const Arg6 &a6)
{
    typedef ArgumentPack6<Arg1, Arg2, Arg3, Arg4, Arg5, Arg6> Pack;
    return Binder<R, R (*)(Sig...), Pack>(f, Pack(a1, a2, a3, a4, a5, a6));
}

template<typename R, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
                     typename Arg6, typename Arg7,
         typename... Sig>
Binder<R, R (*)(Sig...), ArgumentPack7<Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7> >
    bind(R (*f)(Sig...), const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4,
                         const Arg5 &a5, const Arg6 &a6, const Arg7 &a7)
{
    typedef ArgumentPack7<Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7> Pack;
    return Binder<R, R (*)(Sig...), Pack>(f, Pack(a1, a2, a3, a4, a5, a6, a7));
}

/* Function objects */

template<typename R, typename F>
Binder<R, F, ArgumentPack0>
    bind(const F &f)
{
    typedef ArgumentPack0 Pack;
    return Binder<R, F, Pack>(f, Pack());
}

template<typename R, typename Arg1,
         typename F>
Binder<R, F, ArgumentPack1<Arg1> >
    bind(const F &f, const Arg1 &a1)
{
    typedef ArgumentPack1<Arg1> Pack;
    return Binder<R, F, Pack>(f, Pack(a1));
}

template<typename R, typename Arg1, typename Arg2,
         typename F>
Binder<R, F, ArgumentPack2<Arg1, Arg2> >
    bind(const F &f, const Arg1 &a1, const Arg2 &a2)
{
    typedef ArgumentPack2<Arg1, Arg2> Pack;
    return Binder<R, F, Pack>(f, Pack(a1, a2));
}

template<typename R, typename Arg1, typename Arg2, typename Arg3,
         typename F>
Binder<R, F, ArgumentPack3<Arg1, Arg2, Arg3> >
    bind(const F &f, const Arg1 &a1, const Arg2 &a2, const Arg3 &a3)
{
    typedef ArgumentPack3<Arg1, Arg2, Arg3> Pack;
    return Binder<R, F, Pack>(f, Pack(a1, a2, a3));
}

template<typename R, typename Arg1, typename Arg2, typename Arg3, typename Arg4,
         typename F>
Binder<R, F, ArgumentPack4<Arg1, Arg2, Arg3, Arg4> >
    bind(const F &f, const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4)
{
    typedef ArgumentPack4<Arg1, Arg2, Arg3, Arg4> Pack;
    return Binder<R, F, Pack>(f, Pack(a1, a2, a3, a4));
}

template<typename R, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
         typename F>
Binder<R, F, ArgumentPack5<Arg1, Arg2, Arg3, Arg4, Arg5> >
    bind(const F &f, const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4,
                     const Arg5 &a5)
{
    typedef ArgumentPack5<Arg1, Arg2, Arg3, Arg4, Arg5> Pack;
    return Binder<R, F, Pack>(f, Pack(a1, a2, a3, a4, a5));
}

template<typename R, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
                     typename Arg6,
         typename F>
Binder<R, F, ArgumentPack6<Arg1, Arg2, Arg3, Arg4, Arg5, Arg6> >
    bind(const F &f, const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4,
                     const Arg5 &a5, const Arg6 &a6)
{
    typedef ArgumentPack6<Arg1, Arg2, Arg3, Arg4, Arg5, Arg6> Pack;
    return Binder<R, F, Pack>(f, Pack(a1, a2, a3, a4, a5, a6));
}

template<typename R, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5,
                     typename Arg6, typename Arg7,
         typename F>
Binder<R, F, ArgumentPack7<Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7> >
    bind(const F &f, const Arg1 &a1, const Arg2 &a2, const Arg3 &a3, const Arg4 &a4,
                     const Arg5 &a5, const Arg6 &a6, const Arg7 &a7)
{
    typedef ArgumentPack7<Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7> Pack;
    return Binder<R, F, Pack>(f, Pack(a1, a2, a3, a4, a5, a6, a7));
}

} /* namespace Delegate */

extern Delegate::Placeholder<1> _1;
extern Delegate::Placeholder<2> _2;
extern Delegate::Placeholder<3> _3;
extern Delegate::Placeholder<4> _4;
extern Delegate::Placeholder<5> _5;
extern Delegate::Placeholder<6> _6;
extern Delegate::Placeholder<7> _7;

} /* namespace Hypergrace */

#endif /* DELEGATE_BIND_HH_ */
