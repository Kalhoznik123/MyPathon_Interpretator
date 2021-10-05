#include "runtime.h"

#include <cassert>
#include <optional>
#include <sstream>
#include <algorithm>

using namespace std;

namespace runtime
{

    ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
        : data_(std::move(data))
    {
    }

    void ObjectHolder::AssertIsValid() const{
        assert(data_ != nullptr);
    }

    ObjectHolder ObjectHolder::Share(Object &object){
        // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
        return ObjectHolder(std::shared_ptr<Object>(&object, [](auto * /*p*/) { /* do nothing */ }));
    }

    ObjectHolder ObjectHolder::None(){
        return ObjectHolder();
    }

    Object &ObjectHolder::operator*() const{
        AssertIsValid();
        return *Get();
    }

    Object *ObjectHolder::operator->() const{
        AssertIsValid();
        return Get();
    }

    Object *ObjectHolder::Get() const{
        return data_.get();
    }

    ObjectHolder::operator bool() const{
        return Get() != nullptr;
    }

    bool IsTrue(const ObjectHolder &object){
        if (object.TryAs<Number>()){
            return object.TryAs<Number>()->GetValue() != 0;
        }
        else if (object.TryAs<String>()){
            return !object.TryAs<String>()->GetValue().empty();
        }
        else if (object.TryAs<Bool>()){
            return object.TryAs<Bool>()->GetValue() == true;
        }else{
            return false;
        }
    }

    void ClassInstance::Print(std::ostream &os, Context &context){
        if (HasMethod("__str__"s, 0))
            Call("__str__"s, {}, context).Get()->Print(os, context);
        else
            os << this;
    }

    bool ClassInstance::HasMethod(const std::string &method, size_t argument_count) const{
        return class_.GetMethod(method) != nullptr && class_.GetMethod(method)->formal_params.size() == argument_count;
    }

    Closure &ClassInstance::Fields(){
        return fields_;
    }

    const Closure &ClassInstance::Fields() const{
        return fields_;
    }

    ClassInstance::ClassInstance(const Class &cls)
        : class_(cls){
    }

    ObjectHolder ClassInstance::Call(const std::string &method,
                                     const std::vector<ObjectHolder> &actual_args,
                                     Context &context){
        if (!HasMethod(method, actual_args.size())){
            throw std::runtime_error("ERROR: This method not Not implemented"s);
        }
        runtime::Closure args;
        args["self"s] = ObjectHolder::Share(*this);
        for (size_t i = 0; i < actual_args.size(); ++i){
            args[class_.GetMethod(method)->formal_params[i]] = actual_args[i];
        }
        return class_.GetMethod(method)->body->Execute(args, context);
    }

    Class::Class(std::string name, std::vector<Method> methods, const Class *parent)
        : name_(std::move(name))
        , methods_(std::move(methods))
        , parent_(*parent)
    {
    }

    const Method *Class::GetMethod(const std::string &name) const{
        auto it = std::find_if(methods_.begin(), methods_.end(), [&name](const auto &method){ return method.name == name; });

        if (it != methods_.end()){
            return &*it;
        }
        else if (&parent_ != nullptr){
            auto it = parent_.GetMethod(name);
            if (it != nullptr)
                return it;
        }
        return nullptr;
    }

    [[nodiscard]] const std::string &Class::GetName() const{
        return name_;
    }

    void Class::Print(ostream &os, [[maybe_unused]] Context &context){
        os << "Class "s << GetName();
    }

    void Bool::Print(std::ostream &os, [[maybe_unused]] Context &context){
        os << (GetValue() ? "True"sv : "False"sv);
    }

    bool Equal(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context){
        if (lhs.TryAs<Number>() && rhs.TryAs<Number>()){
            return lhs.TryAs<Number>()->GetValue() == rhs.TryAs<Number>()->GetValue();
        }
        else if (lhs.TryAs<String>() && rhs.TryAs<String>()){
            return lhs.TryAs<String>()->GetValue() == rhs.TryAs<String>()->GetValue();
        }
        else if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>()){
            return lhs.TryAs<Bool>()->GetValue() == rhs.TryAs<Bool>()->GetValue();
        }
        else if (!lhs && !rhs){
            return true;
        }
        else if (lhs.TryAs<ClassInstance>() && lhs.TryAs<ClassInstance>()->HasMethod("__eq__"s, 1)){
            return lhs.TryAs<ClassInstance>()->Call("__eq__"s, {rhs}, context).TryAs<Bool>()->GetValue();
        }else{
        throw std::runtime_error("ERROR:These objects cannot be compared"s);
        }
    }

    bool Less(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context)
    {
        if (lhs.TryAs<Number>() && rhs.TryAs<Number>()){
            return lhs.TryAs<Number>()->GetValue() < rhs.TryAs<Number>()->GetValue();
        }
        else if (lhs.TryAs<String>() && rhs.TryAs<String>()){
            return lhs.TryAs<String>()->GetValue() < rhs.TryAs<String>()->GetValue();
        }
        else if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>()){
            return lhs.TryAs<Bool>()->GetValue() < rhs.TryAs<Bool>()->GetValue();
        }
        else if (lhs.TryAs<ClassInstance>() && lhs.TryAs<ClassInstance>()->HasMethod("__lt__"s, 1)){
            return lhs.TryAs<ClassInstance>()->Call("__lt__"s, {rhs}, context).TryAs<Bool>()->GetValue();
        }
        throw std::runtime_error("ERROR:These objects cannot be compared by less"s);
    }

    bool NotEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context){
        return !Equal(lhs, rhs, context);
    }

    bool Greater(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context){
        return !Less(lhs, rhs, context) && !Equal(lhs, rhs, context);
    }

    bool LessOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context){
        return !Greater(lhs, rhs, context);
    }

    bool GreaterOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context){
        return !Less(lhs, rhs, context);
    }

} // namespace runtime
