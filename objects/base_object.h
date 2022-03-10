#pragma once

class Base_object;
// list of parsed object with its names
typedef std::map<std::string, std::shared_ptr<Base_object>> Object_cache;
// keeps hierarchy of transitions in form object_name opened by object name
typedef std::map<std::string, std::string> Objects_opened_by;
// set of unknown objects
typedef std::set<std::string> Objects_unknown;

struct Context
{
  Object_cache object_list;
  Objects_opened_by opened_by;
  Objects_unknown unknown;
  bool batch_mode = false;
};

class Base_object
{
public:
  virtual ~Base_object() { }
  virtual bool is_object_correct() const = 0;
  virtual void store(const std::string& path, const Context& context) = 0;
  virtual std::set<std::string> get_referenced_objects() const = 0;
  virtual void resolve_dependencies(const Context& context) = 0;
  virtual void set_object_name(const std::string& obj_name) = 0;
  virtual std::string get_object_name() const = 0;
};