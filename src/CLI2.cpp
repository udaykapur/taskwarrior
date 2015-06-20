////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2006 - 2015, Paul Beckingham, Federico Hernandez.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// http://www.opensource.org/licenses/mit-license.php
//
////////////////////////////////////////////////////////////////////////////////

#include <cmake.h>
#include <sstream>
#include <algorithm>
#include <stdlib.h>
#include <Context.h>
#include <CLI2.h>
#include <Color.h>
#include <text.h>
#include <util.h>
#include <i18n.h>

extern Context context;

// Overridden by rc.abbreviation.minimum.
int CLI2::minimumMatchLength = 3;

// Alias expansion limit. Any more indicates some kind of error.
static int safetyValveDefault = 10;

////////////////////////////////////////////////////////////////////////////////
A2::A2 ()
: _lextype (Lexer::Type::word)
{
}

////////////////////////////////////////////////////////////////////////////////
A2::A2 (const std::string& raw, Lexer::Type lextype)
{
  _lextype = lextype;
  attribute ("raw", raw);
}

////////////////////////////////////////////////////////////////////////////////
A2::~A2 ()
{
}

////////////////////////////////////////////////////////////////////////////////
A2::A2 (const A2& other)
: _lextype (other._lextype)
, _tags (other._tags)
, _attributes (other._attributes)
{
}

////////////////////////////////////////////////////////////////////////////////
A2& A2::operator= (const A2& other)
{
  if (this != &other)
  {
    _lextype    = other._lextype;
    _tags       = other._tags;
    _attributes = other._attributes;
  }

  return *this;
}

////////////////////////////////////////////////////////////////////////////////
bool A2::hasTag (const std::string& tag) const
{
  if (std::find (_tags.begin (), _tags.end (), tag) != _tags.end ())
    return true;

  return false;
}

////////////////////////////////////////////////////////////////////////////////
void A2::tag (const std::string& tag)
{
  if (! hasTag (tag))
    _tags.push_back (tag);
}

////////////////////////////////////////////////////////////////////////////////
void A2::unTag (const std::string& tag)
{
  for (auto i = _tags.begin (); i != _tags.end (); ++i)
  {
    if (*i == tag)
    {
      _tags.erase (i);
      break;
    }
  }
}

/*
////////////////////////////////////////////////////////////////////////////////
void A2::unTagAll ()
{
  _tags.clear ();
}
*/

////////////////////////////////////////////////////////////////////////////////
// Accessor for attributes.
void A2::attribute (const std::string& name, const std::string& value)
{
  _attributes[name] = value;
}

////////////////////////////////////////////////////////////////////////////////
// Accessor for attributes.
const std::string A2::attribute (const std::string& name) const
{
  // Prevent autovivification.
  auto i = _attributes.find (name);
  if (i != _attributes.end ())
    return i->second;

  return "";
}

////////////////////////////////////////////////////////////////////////////////
const std::string A2::dump () const
{
  std::string output = Lexer::typeToString (_lextype);

  // Dump attributes.
  std::string atts;
  for (auto a = _attributes.begin (); a != _attributes.end (); ++a)
  {
    if (a != _attributes.begin ())
      atts += " ";

    atts += a->first + "='\033[33m" + a->second + "\033[0m'";
  }

  if (atts.length ())
    output += " " + atts;

  // Dump tags.
  std::string tags;
  for (auto& tag : _tags)
  {
    if (tags.length ())
      tags += ' ';

         if (tag == "BINARY")       tags += "\033[1;37;44m"           + tag + "\033[0m";
    else if (tag == "CMD")          tags += "\033[1;37;46m"           + tag + "\033[0m";
    else if (tag == "FILTER")       tags += "\033[1;37;42m"           + tag + "\033[0m";
    else if (tag == "MODIFICATION") tags += "\033[1;37;43m"           + tag + "\033[0m";
    else if (tag == "RC")           tags += "\033[1;37;41m"           + tag + "\033[0m";
    else if (tag == "CONFIG")       tags += "\033[1;37;101m"          + tag + "\033[0m";
    else if (tag == "PSEUDO")       tags += "\033[1;37;45m"           + tag + "\033[0m";
    else if (tag == "?")            tags += "\033[38;5;255;48;5;232m" + tag + "\033[0m";
    else                            tags += "\033[32m"                + tag + "\033[0m";
  }

  if (tags.length ())
    output += ' ' + tags;

  return output;
}

////////////////////////////////////////////////////////////////////////////////
// Static method.
void CLI2::getOverride (int argc, const char** argv, std::string& home, File& rc)
{
  for (int i = 0; i < argc; ++i)
  {
    std::string raw = argv[i];
    if (raw == "--")
      return;

    if (raw.length () > 3 &&
        raw.substr (0, 3) == "rc:")
    {
      rc = raw.substr (3);

      home = ".";
      auto last_slash = rc._data.rfind ("/");
      if (last_slash != std::string::npos)
        home = rc.parent ();

      context.header (format (STRING_PARSER_ALTERNATE_RC, rc._data));

      // Keep looping, because if there are multiple rc:file arguments, the last
      // one should dominate.
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Look for CONFIG data.location and initialize a Path object.
// Static method.
void CLI2::getDataLocation (int argc, const char** argv, Path& data)
{
  std::string location = context.config.get ("data.location");
  if (location != "")
    data = location;

  for (int i = 0; i < argc; ++i)
  {
    std::string raw = argv[i];
    if (raw == "--")
      break;

    if (raw.length () > 17 &&
        raw.substr (0, 16) == "rc.data.location")
    {
      data = Directory (raw.substr (17));
      context.header (format (STRING_PARSER_ALTERNATE_DATA, (std::string) data));

      // Keep looping, because if there are multiple rc:file arguments, the last
      // one should dominate.
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// Static method.
void CLI2::applyOverrides (int argc, const char** argv)
{
  for (int i = 0; i < argc; ++i)
  {
    std::string raw = argv[i];
    if (raw == "--")
      break;

    if (raw.length () > 3 &&
        raw.substr (0, 3) == "rc.")
    {
      auto sep = raw.find ('=', 3);
      if (sep == std::string::npos)
        sep = raw.find (':', 3);
      if (sep != std::string::npos)
      {
        std::string name  = raw.substr (3, sep - 3);
        std::string value = raw.substr (sep + 1);
        context.config.set (name, value);
        context.footnote (format (STRING_PARSER_OVERRIDE_RC, name, value));
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
CLI2::CLI2 ()
{
}

////////////////////////////////////////////////////////////////////////////////
CLI2::~CLI2 ()
{
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::alias (const std::string& name, const std::string& value)
{
  _aliases.insert (std::pair <std::string, std::string> (name, value));
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::entity (const std::string& category, const std::string& name)
{
  // Walk the list of entities for category.
  auto c = _entities.equal_range (category);
  for (auto e = c.first; e != c.second; ++e)
    if (e->second == name)
      return;

  // The category/name pair was not found, therefore add it.
  _entities.insert (std::pair <std::string, std::string> (category, name));
}

////////////////////////////////////////////////////////////////////////////////
// Capture a single argument.
void CLI2::add (const std::string& argument)
{
  _original_args.push_back (argument);

  // Adding a new argument invalidates prior analysis.
  _args.clear ();
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::handleArg0 ()
{
  // Capture arg0 separately, because it is the command that was run, and could
  // need special handling.
  std::string raw = _original_args[0];
  A2 a (raw, Lexer::Type::word);
  a.tag ("BINARY");

  std::string basename = "task";
  auto slash = raw.rfind ('/');
  if (slash != std::string::npos)
    basename = raw.substr (slash + 1);

  a.attribute ("basename", basename);
  if (basename == "cal" ||
      basename == "calendar")
  {
    _args.push_back (a);

    A2 cal ("calendar", Lexer::Type::word);
    _args.push_back (cal);
  }
  else if (basename == "task" ||
           basename == "tw" ||
           basename == "t")
  {
    _args.push_back (a);
  }
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::lexArguments ()
{
  // Note: Starts interating at index 1, because ::handleArg0 has already
  //       processed it.
  for (unsigned int i = 1; i < _original_args.size (); ++i)
  {
    std::string lexeme;
    Lexer::Type type;
    Lexer lex (_original_args[i]);
    lex.ambiguity (false);

    while (lex.token (lexeme, type))
      _args.push_back (A2 (lexeme, type));
  }

  if (context.config.getInteger ("debug.parser") >= 3)
    context.debug (dump ("CLI2::analyze lexArguments"));
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::handleTerminator ()
{
  bool changes = false;
  bool terminated = false;
  std::vector <A2> reconstructed;
  for (auto& a : _args)
  {
    if (a._lextype == Lexer::Type::separator)
    {
      terminated = true;
      changes = true;
    }
    else
    {
      if (terminated)
        a._lextype = Lexer::Type::word;

      reconstructed.push_back (a);
    }
  }

  if (changes)
  {
    _args = reconstructed;

    if (context.config.getInteger ("debug.parser") >= 3)
      context.debug (dump ("CLI2::analyze handleTerminator"));
  }
}

////////////////////////////////////////////////////////////////////////////////
// Intended to be called after ::add() to perform the final analysis.
void CLI2::analyze ()
{
  if (context.config.getInteger ("debug.parser") >= 3)
    context.debug (dump ("CLI2::analyze start"));

  // Process _original_args.
  _args.clear ();
  handleArg0 ();
  lexArguments ();
  handleTerminator ();

  // Process _args.
  aliasExpansion ();
  findOverrides ();
  if (! findCommand ())
  {
    defaultCommand ();
    if (! findCommand ())
      throw std::string (STRING_TRIVIAL_INPUT);
  }

  if (context.config.getInteger ("debug.parser") >= 3)
    context.debug (dump ("CLI2::analyze end"));
}

/*
////////////////////////////////////////////////////////////////////////////////
// There are situations where a context filter is applied. This method
// determines whether one applies, and if so, applies it.
void CLI2::addContextFilter ()
{
  // Detect if any context is set, and bail out if not
  std::string contextName = context.config.get ("context");
  if (contextName == "")
  {
    context.debug ("No context applied.");
    return;
  }

  // Detect if UUID or ID is set, and bail out
  for (auto& a : _args)
  {
    // TODO This looks wrong.
    if (a.hasTag ("FILTER") &&
        a.hasTag ("ATTRIBUTE") &&
        ! a.hasTag ("TERMINATED") &&
        ! a.hasTag ("WORD") &&
        (a.attribute ("raw") == "id" || a.attribute ("raw") == "uuid"))
    {
      context.debug (format ("UUID/ID lexeme found '{1}', not applying context.", a.attribute ("raw")));
      return;
    }
  }

  // Apply context
  context.debug ("Applying context: " + contextName);
  std::string contextFilter = context.config.get ("context." + contextName);

  if (contextFilter == "")
    context.debug ("Context '" + contextName + "' not defined.");
  else
  {
    addRawFilter ("( " + contextFilter + " )");
    if (context.verbose ("context"))
      context.footnote (format ("Context '{1}' set. Use 'task context none' to remove.", contextName));
  }
}

////////////////////////////////////////////////////////////////////////////////
// Process raw string into parsed filter.
void CLI2::addRawFilter (const std::string& arg)
{
  std::string lexeme;
  Lexer::Type type;
  Lexer lex (arg);
  lex.ambiguity (false);

  while (lex.token (lexeme, type))
    add (lexeme);
}
*/

////////////////////////////////////////////////////////////////////////////////
// Parset the commnad line, identifiying filter components, expanding syntactic
// sugar as necessary.
void CLI2::prepareFilter (bool applyContext)
{
  // Clear and re-populate.
  _id_ranges.clear ();
  _uuid_list.clear ();

  // Classify FILTER and MODIFICATION args, based on CMD and READCMD/WRITECMD.
  bool changes = false;
  bool foundCommand = false;
  bool readOnly = false;

  for (auto& a : _args)
  {
    if (a.hasTag ("CMD"))
    {
      foundCommand = true;
      if (a.hasTag ("READCMD"))
        readOnly = true;
    }
    else if (a.hasTag ("BINARY") ||
             a.hasTag ("RC")     ||
             a.hasTag ("CONFIG"))
    {
      // NOP.
    }
    else if (foundCommand && ! readOnly)
    {
      a.tag ("MODIFICATION");
      changes = true;
    }
    else if (!foundCommand || (foundCommand && readOnly))
    {
      a.tag ("FILTER");
      changes = true;
    }
  }

  if (changes &&
      context.config.getInteger ("debug.parser") >= 3)
    context.debug (dump ("CLI2::prepareFilter categorize"));

  // Remove all the syntactic sugar for FILTERs.
  changes = false;
  findIDs ();
  findUUIDs ();
  insertIDExpr ();
  desugarFilterTags ();
  findStrayModifications ();
/*
  desugarFilterAttributes ();
  desugarFilterAttributeModifiers ();
  desugarFilterPatterns ();
  findAttributes ();
  desugarFilterPlainArgs ();
  insertJunctions ();                 // Deliberately after all desugar calls.

  // Decompose the elements for MODIFICATIONs.
  decomposeModAttributes ();
  decomposeModAttributeModifiers ();
  decomposeModTags ();
  decomposeModSubstitutions ();
*/

  if (changes &&
      context.config.getInteger ("debug.parser") >= 3)
    context.debug (dump ("CLI2::prepareFilter desugar"));
}

////////////////////////////////////////////////////////////////////////////////
// Get the original command line arguments, in pristine condition, but skipping:
//   - BINARY
//   - CMD
//   - RC
//   - CONFIG
//   - --
const std::vector <std::string> CLI2::getWords (bool filtered)
{
  auto binary = getBinary ();
  auto command = getCommand ();
  auto commandRaw = getCommand (false);

  std::vector <std::string> words;
  for (auto& a : _original_args)
  {
    if (a != binary         &&
        a != command        &&
        a != commandRaw     &&
        a != "--")
    {
      if (! filtered ||
          (a.find ("rc:") != 0 &&
           a.find ("rc.") != 0))
      {
        words.push_back (a);
      }
    }
  }

  if (context.config.getInteger ("debug.parser") >= 3)
  {
    Color colorOrigArgs ("gray10 on gray4");
    std::string message = " ";
    for (auto& word : words)
      message += colorOrigArgs.colorize (word) + " ";
    context.debug ("CLI2::getWords" + message);
  }

  return words;
}

////////////////////////////////////////////////////////////////////////////////
// Search for 'value' in _entities category, return canonicalized value.
bool CLI2::canonicalize (
  std::string& canonicalized,
  const std::string& category,
  const std::string& value) const
{
  // Extract a list of entities for category.
  std::vector <std::string> options;
  auto c = _entities.equal_range (category);
  for (auto e = c.first; e != c.second; ++e)
  {
    // Shortcut: if an exact match is found, success.
    if (value == e->second)
    {
      canonicalized = value;
      return true;
    }

    options.push_back (e->second);
  }

  // Match against the options, throw away results.
  std::vector <std::string> matches;
  if (autoComplete (value, options, matches, minimumMatchLength) == 1)
  {
    canonicalized = matches[0];
    return true;
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
std::string CLI2::getBinary () const
{
  if (_args.size ())
    return _args[0].attribute ("raw");

  return "";
}

////////////////////////////////////////////////////////////////////////////////
std::string CLI2::getCommand (bool canonical) const
{
  for (auto& a : _args)
    if (a.hasTag ("CMD"))
      return a.attribute (canonical ? "canonical" : "raw");

  return "";
}

/*
////////////////////////////////////////////////////////////////////////////////
std::string CLI2::getLimit () const
{
  for (auto& a : _args)
    if (a.hasTag ("PSEUDO") &&
        a.attribute ("canonical") == "limit")
      return a.attribute ("value");

  return "0";
}
*/

////////////////////////////////////////////////////////////////////////////////
const std::string CLI2::dump (const std::string& title) const
{
  std::stringstream out;

  out << "\033[1m" << title << "\033[0m\n"
      << "  _original_args\n    ";

  Color colorArgs ("gray10 on gray4");
  for (auto i = _original_args.begin (); i != _original_args.end (); ++i)
  {
    if (i != _original_args.begin ())
      out << ' ';
    out << colorArgs.colorize (*i);
  }
  out << "\n";

  if (_args.size ())
  {
    out << "  _args\n";
    for (auto& a : _args)
      out << "    " << a.dump () << "\n";
  }

  if (_id_ranges.size ())
  {
    out << "  _id_ranges\n    ";
    for (auto& range : _id_ranges)
    {
      if (range.first != range.second)
        out << colorArgs.colorize (range.first + "-" + range.second) << " ";
      else
        out << colorArgs.colorize (range.first) << " ";
    }

    out << "\n";
  }

  if (_uuid_list.size ())
  {
    out << "  _uuid_list\n    ";
    for (auto& uuid : _uuid_list)
      out << colorArgs.colorize (uuid) << " ";

    out << "\n";
  }

  return out.str ();
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::aliasExpansion ()
{
  bool changes = false;
  bool action;
  int counter = 0;
  do
  {
    action = false;
    std::vector <A2> reconstructed;

    std::string raw;
    for (auto& i : _args)
    {
      raw = i.attribute ("raw");
      if (_aliases.find (raw) != _aliases.end ())
      {
        for (auto& l : Lexer::split (_aliases[raw]))
        {
          A2 a (l, Lexer::Type::word);
          a.tag ("ALIAS");
          a.tag ("LEX");
          reconstructed.push_back (a);
        }

        action = true;
        changes = true;
      }
      else
        reconstructed.push_back (i);
    }

    _args = reconstructed;

    std::vector <std::string> reconstructedOriginals;
    for (auto& i : _original_args)
    {
      if (_aliases.find (i) != _aliases.end ())
      {
        for (auto& l : Lexer::split (_aliases[i]))
          reconstructedOriginals.push_back (l);

        action = true;
        changes = true;
      }
      else
        reconstructedOriginals.push_back (i);
    }

    _original_args = reconstructedOriginals;
  }
  while (action && counter++ < safetyValveDefault);

  if (counter >= safetyValveDefault)
    context.debug (format (STRING_PARSER_ALIAS_NEST, safetyValveDefault));

  if (changes &&
      context.config.getInteger ("debug.parser") >= 3)
    context.debug (dump ("CLI2::analyze aliasExpansion"));
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::findOverrides ()
{
  bool changes = false;
  std::string raw;

  for (auto& a : _args)
  {
    raw = a.attribute ("raw");
    if (raw.length () > 3 &&
        raw.find ("rc:") == 0)
    {
      a.tag ("RC");
      a.attribute ("file", raw.substr (3));
      changes = true;
    }
    else if (raw.length () > 3 &&
             raw.find ("rc.") == 0)
    {
      auto sep = raw.find ('=', 3);
      if (sep == std::string::npos)
        sep = raw.find (':', 3);
      if (sep != std::string::npos)
      {
        a.tag ("CONFIG");
        a.attribute ("name", raw.substr (3, sep - 3));
        a.attribute ("value", raw.substr (sep + 1));
        changes = true;
      }
    }
  }

  if (changes &&
      context.config.getInteger ("debug.parser") >= 3)
    context.debug (dump ("CLI2::analyze findOverrides"));
}

////////////////////////////////////////////////////////////////////////////////
bool CLI2::findCommand ()
{
  for (auto& a : _args)
  {
    std::string canonical;
    if (canonicalize (canonical, "cmd", a.attribute ("raw")))
    {
      a.attribute ("canonical", canonical);
      a.tag ("CMD");

      bool readOnly = ! exactMatch ("writecmd", canonical);
      a.tag (readOnly ? "READCMD" : "WRITECMD");

      if (context.config.getInteger ("debug.parser") >= 3)
        context.debug (dump ("CLI2::analyze findCommand"));

      // Stop and indicate command found.
      return true;
    }
  }

  // Indicate command not found.
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// Search for exact 'value' in _entities category.
bool CLI2::exactMatch (
  const std::string& category,
  const std::string& value) const
{
  // Extract a list of entities for category.
  auto c = _entities.equal_range (category);
  for (auto e = c.first; e != c.second; ++e)
    if (value == e->second)
      return true;

  return false;
}

////////////////////////////////////////////////////////////////////////////////
// +tag --> tags _hastag_ tag
// -tag --> tags _notag_ tag
void CLI2::desugarFilterTags ()
{
  bool changes = false;
  std::vector <A2> reconstructed;
  for (auto& a : _args)
  {
    if (a._lextype == Lexer::Type::tag)
    {
      changes = true;

      A2 left ("tags", Lexer::Type::dom);
      left.tag ("FILTER");
      reconstructed.push_back (left);

      std::string raw = a.attribute ("raw");

      A2 op (raw[0] == '+' ? "_hastag_" : "_notag_", Lexer::Type::op);
      op.tag ("FILTER");
      reconstructed.push_back (op);

      A2 right ("" + raw.substr (1) + "", Lexer::Type::string);
      right.tag ("FILTER");
      reconstructed.push_back (right);
    }
    else
      reconstructed.push_back (a);
  }

  if (changes)
  {
    _args = reconstructed;

    if (context.config.getInteger ("debug.parser") >= 3)
      context.debug (dump ("CLI2::prepareFilter desugarFilterTags"));
  }
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::findStrayModifications ()
{
  bool changes = false;

  std::string command = getCommand ();
  if (command == "add" ||
      command == "log")
  {
    for (auto& a : _args)
    {
      if (a.hasTag ("FILTER"))
      {
        a.unTag ("FILTER");
        a.tag ("MODIFICATION");
        changes = true;
      }
    }
  }

  if (changes)
    if (context.config.getInteger ("debug.parser") >= 3)
      context.debug (dump ("CLI2::prepareFilter findStrayModifications"));
}

/*
////////////////////////////////////////////////////////////////////////////////
// <name>:['"][<value>]['"] --> name = value
void CLI2::desugarFilterAttributes ()
{
  bool changes = false;
  std::vector <A> reconstructed;
  for (auto& a : _args)
  {
    if (a.hasTag ("FILTER"))
    {
      // Look for a valid attribute name.
      bool found = false;
      Nibbler n (a.attribute ("raw"));
      std::string name;
      if (n.getName (name) &&
          name.length ())
      {
        if (n.skip (':') ||
            n.skip ('='))
        {
          std::string value;
          if (n.getQuoted   ('"', value)  ||
              n.getQuoted   ('\'', value) ||
              n.getUntilEOS (value)       ||
              n.depleted ())
          {
            if (value == "")
              value = "''";

            std::string canonical;
            if (canonicalize (canonical, "uda", name))
            {
              A lhs ("argUDA", name);
              lhs.attribute ("name", canonical);
              lhs.tag ("UDA");
              lhs.tag ("ATTRIBUTE");
              lhs.tag ("FILTER");

              A op ("argUDA", "=");
              op.tag ("OP");
              op.tag ("FILTER");

              A rhs ("argUDA", value);
              rhs.tag ("LITERAL");
              rhs.tag ("FILTER");

              reconstructed.push_back (lhs);
              reconstructed.push_back (op);
              reconstructed.push_back (rhs);
              found = true;
            }

            else if (canonicalize (canonical, "pseudo", name))
            {
              A lhs ("argPseudo", a.attribute ("raw"));
              lhs.attribute ("canonical", canonical);
              lhs.attribute ("value", value);
              lhs.tag ("PSEUDO");
              reconstructed.push_back (lhs);
              found = true;
            }

            else if (canonicalize (canonical, "attribute", name))
            {
              A lhs ("argAtt", name);
              lhs.attribute ("name", canonical);
              lhs.tag ("ATTRIBUTE");
              lhs.tag ("FILTER");

              std::string operatorLiteral = "=";
              if (canonical == "status")
                operatorLiteral = "==";

              A op ("argAtt", operatorLiteral);
              op.tag ("OP");
              op.tag ("FILTER");

              A rhs ("argAtt", value);
              rhs.tag ("LITERAL");
              rhs.tag ("FILTER");

              reconstructed.push_back (lhs);
              reconstructed.push_back (op);
              reconstructed.push_back (rhs);
              found = true;
            }
          }
        }
      }

      if (found)
        changes = true;
      else
        reconstructed.push_back (a);
    }
    else
      reconstructed.push_back (a);
  }

  if (changes)
  {
    _args = reconstructed;

    if (context.config.getInteger ("debug.parser") >= 3)
      context.debug (dump ("CLI2::analyze desugarFilterAttributes"));
  }
}

////////////////////////////////////////////////////////////////////////////////
// <name>.[~]<mod>[:=]['"]<value>['"] --> name <op> value
void CLI2::desugarFilterAttributeModifiers ()
{
  bool changes = false;
  std::vector <A> reconstructed;
  for (auto& a : _args)
  {
    if (a.hasTag ("FILTER"))
    {
      // Look for a valid attribute name.
      bool found = false;
      Nibbler n (a.attribute ("raw"));
      std::string name;
      if (n.getUntil (".", name) &&
          name.length ())
      {
        std::string canonical;
        if (canonicalize (canonical, "attribute", name) ||
            canonicalize (canonical, "uda",       name))
        {
          if (n.skip ('.'))
          {
            std::string sense = "+";
            if (n.skip ('~'))
              sense = "-";

            std::string modifier;
            n.getUntilOneOf (":=", modifier);

            if (n.skip (':') ||
                n.skip ('='))
            {
              std::string value;
              if (n.getQuoted   ('"', value)  ||
                  n.getQuoted   ('\'', value) ||
                  n.getUntilEOS (value)       ||
                  n.depleted ())
              {
                if (value == "")
                  value = "''";

                A lhs ("argAttMod", name);
                lhs.tag ("ATTMOD");
                lhs.tag ("FILTER");

                lhs.attribute ("name", canonical);
                lhs.attribute ("modifier", modifier);
                lhs.attribute ("sense", sense);

                A op ("argAttmod", "");
                op.tag ("FILTER");
                op.tag ("OP");

                A rhs ("argAttMod", "");
                rhs.tag ("FILTER");

                if (modifier == "before" || modifier == "under" || modifier == "below")
                {
                  op.attribute ("raw", "<");
                  rhs.attribute ("raw", value);
                  rhs.tag ("LITERAL");
                }
                else if (modifier == "after" || modifier == "over" || modifier == "above")
                {
                  op.attribute ("raw", ">");
                  rhs.attribute ("raw", value);
                  rhs.tag ("LITERAL");
                }
                else if (modifier == "none")
                {
                  op.attribute ("raw", "==");
                  rhs.attribute ("raw", "''");
                  rhs.tag ("LITERAL");
                }
                else if (modifier == "any")
                {
                  op.attribute ("raw", "!=");
                  rhs.attribute ("raw", "''");
                  rhs.tag ("LITERAL");
                }
                else if (modifier == "is" || modifier == "equals")
                {
                  op.attribute ("raw", "==");
                  rhs.attribute ("raw", "'" + value + "'");
                  rhs.tag ("LITERAL");
                }
                else if (modifier == "isnt" || modifier == "not")
                {
                  op.attribute ("raw", "!==");
                  rhs.attribute ("raw", "'" + value + "'");
                  rhs.tag ("LITERAL");
                }
                else if (modifier == "has" || modifier == "contains")
                {
                  op.attribute ("raw", "~");
                  rhs.attribute ("raw", "'" + value + "'");
                  rhs.tag ("LITERAL");
                }
                else if (modifier == "hasnt")
                {
                  op.attribute ("raw", "!~");
                  rhs.attribute ("raw", "'" + value + "'");
                  rhs.tag ("LITERAL");
                }
                else if (modifier == "startswith" || modifier == "left")
                {
                  op.attribute ("raw", "~");
                  rhs.attribute ("raw", "'^" + value + "'");
                  rhs.tag ("REGEX");
                }
                else if (modifier == "endswith" || modifier == "right")
                {
                  op.attribute ("raw", "~");
                  rhs.attribute ("raw", "'" + value + "$'");
                  rhs.tag ("REGEX");
                }
                else if (modifier == "word")
                {
                  op.attribute ("raw", "~");
#if defined (DARWIN)
                  rhs.attribute ("raw", value);
#elif defined (SOLARIS)
                  rhs.attribute ("raw", "'\\<" + value + "\\>'");
#else
                  rhs.attribute ("raw", "'\\b" + value + "\\b'");
#endif
                  rhs.tag ("REGEX");
                }
                else if (modifier == "noword")
                {
                  op.attribute ("raw", "!~");
#if defined (DARWIN)
                  rhs.attribute ("raw", value);
#elif defined (SOLARIS)
                  rhs.attribute ("raw", "'\\<" + value + "\\>'");
#else
                  rhs.attribute ("raw", "'\\b" + value + "\\b'");
#endif
                  rhs.tag ("REGEX");
                }
                else
                  throw format (STRING_PARSER_UNKNOWN_ATTMOD, modifier);

                reconstructed.push_back (lhs);
                reconstructed.push_back (op);
                reconstructed.push_back (rhs);
                found = true;
              }
            }
          }
        }
      }

      if (found)
        changes = true;
      else
        reconstructed.push_back (a);
    }
    else
      reconstructed.push_back (a);
  }

  if (changes)
  {
    _args = reconstructed;

    if (context.config.getInteger ("debug.parser") >= 3)
      context.debug (dump ("CLI2::analyze desugarFilterAttributeModifiers"));
  }
}

////////////////////////////////////////////////////////////////////////////////
// /pattern/ --> description ~ 'pattern'
void CLI2::desugarFilterPatterns ()
{
  bool changes = false;
  std::vector <A> reconstructed;
  for (auto& a : _args)
  {
    if (a.hasTag ("FILTER"))
    {
      Nibbler n (a.attribute ("raw"));
      std::string pattern;

      if (n.getQuoted ('/', pattern) &&
          n.depleted () &&
          pattern.length () > 0)
      {
        A lhs ("argPattern", "description");
        lhs.tag ("ATTRIBUTE");
        lhs.tag ("FILTER");
        reconstructed.push_back (lhs);

        A op ("argPattern", "~");
        op.tag ("OP");
        op.tag ("FILTER");
        reconstructed.push_back (op);

        A rhs ("argPattern", "'" + pattern + "'");
        rhs.tag ("LITERAL");
        rhs.tag ("FILTER");
        reconstructed.push_back (rhs);
        changes = true;
      }
      else
        reconstructed.push_back (a);
    }
    else
      reconstructed.push_back (a);
  }

  if (changes)
  {
    _args = reconstructed;

    if (context.config.getInteger ("debug.parser") >= 3)
      context.debug (dump ("CLI2::analyze desugarFilterPatterns"));
  }
}
*/

////////////////////////////////////////////////////////////////////////////////
// An ID sequence can be:
//
//   a single ID:          1
//   a list of IDs:        1,3,5
//   a list of IDs:        1 3 5
//   a range:              5-10
//   or a combination:     1,3,5-10 12
//
void CLI2::findIDs ()
{
  for (auto& a : _args)
  {
    if (a.hasTag ("FILTER"))
    {
      if (a._lextype == Lexer::Type::number)
      {
        std::string number = a.attribute ("raw");
        _id_ranges.push_back (std::pair <std::string, std::string> (number, number));
      }
      else if (a._lextype == Lexer::Type::set)
      {
        // Split the ID list into elements.
        std::vector <std::string> elements;
        split (elements, a.attribute ("raw"), ',');

        for (auto& element : elements)
        {
          auto hyphen = element.find ("-");
          if (hyphen != std::string::npos)
          {
            _id_ranges.push_back (std::pair <std::string, std::string> (element.substr (0, hyphen), element.substr (hyphen + 1)));
          }
          else
          {
            _id_ranges.push_back (std::pair <std::string, std::string> (element, element));
          }
        }
      }
    }
  }

  if (_id_ranges.size ())
    if (context.config.getInteger ("debug.parser") >= 3)
      context.debug (dump ("CLI2::prepareFilter findIDs"));
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::findUUIDs ()
{
  for (auto& a : _args)
  {
    if (a.hasTag ("FILTER") &&
        a._lextype == Lexer::Type::uuid)
    {
      a.tag ("UUID");
      _uuid_list.push_back (a.attribute ("raw"));
    }
  }

  if (_uuid_list.size ())
    if (context.config.getInteger ("debug.parser") >= 3)
      context.debug (dump ("CLI2::prepareFilter findUUIDs"));
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::insertIDExpr ()
{
  // TODO Strip out Lexer::Type::list from between Lexer::Type::uuid's.

  // Find the *first* occurence of lexer type set/number/uuid, and replace it
  // with a synthesized expression. All other occurences are eaten.
  bool changes = false;
  bool foundID = false;
  std::vector <A2> reconstructed;
  for (auto& a : _args)
  {
    if (a.hasTag ("FILTER") &&
        (a._lextype == Lexer::Type::set ||
         a._lextype == Lexer::Type::number ||
         a._lextype == Lexer::Type::uuid))
    {
      if (! foundID)
      {
        foundID = true;
        changes = true;

        // Construct a single sequence that represents all _id_ranges and
        // _uuid_list in one clause. This is essentially converting this:
        //
        //   1,2-3 uuid,uuid uuid 4
        //
        // into:
        //
        //   (
        //        ( id == 1 )
        //     or ( ( id >= 2 ) and ( id <= 3 ) )
        //     or ( id == 4 )
        //     or ( uuid = $UUID )
        //     or ( uuid = $UUID )
        //   )

        // Building block operators.
        A2 openParen  ("(",   Lexer::Type::op);  openParen.tag  ("FILTER");
        A2 closeParen (")",   Lexer::Type::op);  closeParen.tag ("FILTER");
        A2 opOr       ("or",  Lexer::Type::op);  opOr.tag       ("FILTER");
        A2 opAnd      ("and", Lexer::Type::op);  opAnd.tag      ("FILTER");
        A2 opSimilar  ("=",   Lexer::Type::op);  opSimilar.tag  ("FILTER");
        A2 opEqual    ("==",  Lexer::Type::op);  opEqual.tag    ("FILTER");
        A2 opGTE      (">=",  Lexer::Type::op);  opGTE.tag      ("FILTER");
        A2 opLTE      ("<=",  Lexer::Type::op);  opLTE.tag      ("FILTER");

        // Building block attributes.
        A2 argID ("id", Lexer::Type::dom);
        argID.tag ("FILTER");

        A2 argUUID ("uuid", Lexer::Type::uuid);
        argUUID.tag ("FILTER");

        reconstructed.push_back (openParen);

        // Add all ID ranges.
        for (auto r = _id_ranges.begin (); r != _id_ranges.end (); ++r)
        {
          if (r != _id_ranges.begin ())
            reconstructed.push_back (opOr);

          if (r->first == r->second)
          {
            reconstructed.push_back (openParen);
            reconstructed.push_back (argID);
            reconstructed.push_back (opEqual);

            A2 value (r->first, Lexer::Type::number);
            value.tag ("FILTER");
            reconstructed.push_back (value);

            reconstructed.push_back (closeParen);
          }
          else
          {
            reconstructed.push_back (openParen);
            reconstructed.push_back (argID);
            reconstructed.push_back (opGTE);

            A2 startValue (r->first, Lexer::Type::number);
            startValue.tag ("FILTER");
            reconstructed.push_back (startValue);

            reconstructed.push_back (opAnd);
            reconstructed.push_back (argID);
            reconstructed.push_back (opLTE);

            A2 endValue (r->second, Lexer::Type::number);
            endValue.tag ("FILTER");
            reconstructed.push_back (endValue);

            reconstructed.push_back (closeParen);
          }
        }

        // Combine the ID and UUID sections with 'or'.
        if (_id_ranges.size () &&
            _uuid_list.size ())
          reconstructed.push_back (opOr);

        // Add all UUID list items.
        for (auto u = _uuid_list.begin (); u != _uuid_list.end (); ++u)
        {
          if (u != _uuid_list.begin ())
            reconstructed.push_back (opOr);

          reconstructed.push_back (openParen);
          reconstructed.push_back (argUUID);
          reconstructed.push_back (opSimilar);

          A2 value (*u, Lexer::Type::string);
          value.tag ("FILTER");
          reconstructed.push_back (value);

          reconstructed.push_back (closeParen);
        }

        reconstructed.push_back (closeParen);
      }

      // No 'else' because all set/number/uuid args but the first are removed.
    }
    else
      reconstructed.push_back (a);
  }

  if (changes)
  {
    _args = reconstructed;

    if (context.config.getInteger ("debug.parser") >= 3)
      context.debug (dump ("CLI2::prepareFilter insertIDExpr"));
  }
}

/*
////////////////////////////////////////////////////////////////////////////////
void CLI2::desugarFilterPlainArgs ()
{
  bool changes = false;
  std::vector <A> reconstructed;
  auto prev = _args.begin ();
  for (auto a = _args.begin (); a != _args.end (); ++a)
  {
    if (a != prev                 && // Not the first arg.
        ! prev->hasTag ("OP")     && // An OP before protects the arg.
        a->hasTag ("FILTER")      &&
        ! a->hasTag ("ATTRIBUTE") &&
        ! a->hasTag ("ATTMOD")    &&
        ! a->hasTag ("OP")        &&
        ! a->hasTag ("REGEX")     &&
        ! a->hasTag ("LITERAL"))
    {
      A lhs ("argPattern", "description");
      lhs.tag ("ATTRIBUTE");
      lhs.tag ("FILTER");
      reconstructed.push_back (lhs);

      A op ("argPattern", "~");
      op.tag ("OP");
      op.tag ("FILTER");
      reconstructed.push_back (op);

      std::string pattern = a->attribute ("raw");
      Lexer::dequote (pattern);
      A rhs ("argPattern", "'" + pattern + "'");
      rhs.tag ("LITERAL");
      rhs.tag ("FILTER");
      reconstructed.push_back (rhs);
      changes = true;
    }
    else
      reconstructed.push_back (*a);

    prev = a;
  }

  if (changes)
  {
    _args = reconstructed;

    if (context.config.getInteger ("debug.parser") >= 3)
      context.debug (dump ("CLI2::analyze desugarFilterPlainArgs"));
  }
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::findAttributes ()
{
  // Extract a list of entities for category.
  std::vector <std::string> options;
  auto c = _entities.equal_range ("attribute");
  for (auto e = c.first; e != c.second; ++e)
    options.push_back (e->second);

  // Walk the arguments and tag as OP.
  bool changes = false;
  for (auto& a : _args)
    if (a.hasTag ("FILTER"))
      if (std::find (options.begin (), options.end (), a.attribute ("raw")) != options.end ())
        if (! a.hasTag ("ATTRIBUTE"))
        {
          a.tag ("ATTRIBUTE");
          changes = true;
        }

  if (changes &&
      context.config.getInteger ("debug.parser") >= 3)
    context.debug (dump ("CLI2::analyze findAttributes"));
}

////////////////////////////////////////////////////////////////////////////////
// Two consecutive FILTER, non-OP arguments that are not "(" or ")" need an
// "and" operator inserted between them.
//
//   ) <non-op>         -->  ) and <non-op>
//   <non-op> (         -->  <non-op> <and> (
//   ) (                -->  ) and (
//   <non-op> <non-op>  -->  <non-op> and <non-op>
//
void CLI2::insertJunctions ()
{
  bool changes = false;
  std::vector <A> reconstructed;
  auto prev = _args.begin ();
  for (auto a = _args.begin (); a != _args.end (); ++a)
  {
    if (a->hasTag ("FILTER"))
    {
      // The prev iterator should be the first FILTER arg.
      if (prev == _args.begin ())
        prev = a;

      // Insert AND between terms.
      else if (a != prev)
      {
        if ((! prev->hasTag ("OP")          && a->attribute ("raw") == "(") ||
            (! prev->hasTag ("OP")          && ! a->hasTag ("OP"))          ||
            (prev->attribute ("raw") == ")" && ! a->hasTag ("OP"))          ||
            (prev->attribute ("raw") == ")" && a->attribute ("raw") == "("))
        {
          A opOr ("argOp", "and");
          opOr.tag ("FILTER");
          opOr.tag ("OP");
          reconstructed.push_back (opOr);
          changes = true;
        }
      }

      // Previous FILTER arg.
      prev = a;
    }

    reconstructed.push_back (*a);
  }

  if (changes)
  {
    _args = reconstructed;

    if (context.config.getInteger ("debug.parser") >= 3)
      context.debug (dump ("CLI2::analyze insertJunctions"));
  }
}
*/
////////////////////////////////////////////////////////////////////////////////
void CLI2::defaultCommand ()
{
  // Scan the top-level branches for evidence of ID, UUID, overrides and other
  // arguments.
  bool changes          = false;
  bool found_command    = false;
  bool found_sequence   = false;

  for (auto& a : _args)
  {
    std::string raw = a.attribute ("raw");

    if (a.hasTag ("CMD"))
      found_command = true;

    if (a._lextype == Lexer::Type::uuid ||
        a._lextype == Lexer::Type::number)
      found_sequence = true;
  }

  // If no command was specified, then a command will be inserted.
  if (! found_command)
  {
    // Default command.
    if (! found_sequence)
    {
      // Apply overrides, if any.
      std::string defaultCommand = context.config.get ("default.command");
      if (defaultCommand != "")
      {
        // Modify _args, _original_args to be:
        //   <args0> [<def0> ...] <args1> [...]

        std::vector <std::string> reconstructedOriginals {_original_args[0]};
        std::vector <A2> reconstructed {_args[0]};

        std::string lexeme;
        Lexer::Type type;
        Lexer lex (defaultCommand);
        lex.ambiguity (false);

        while (lex.token (lexeme, type))
        {
          reconstructedOriginals.push_back (lexeme);

          A2 cmd (lexeme, type);
          cmd.tag ("DEFAULT");
          reconstructed.push_back (cmd);
        }

        for (unsigned int i = 1; i < _original_args.size (); ++i)
          reconstructedOriginals.push_back (_original_args[i]);

        for (unsigned int i = 1; i < _args.size (); ++i)
          reconstructed.push_back (_args[i]);

        _original_args = reconstructedOriginals;
        _args = reconstructed;
        changes = true;
      }
    }
    else
    {
      A2 info ("information", Lexer::Type::word);
      info.tag ("DEFAULT");
      _args.push_back (info);
      changes = true;
    }
  }

  if (changes &&
      context.config.getInteger ("debug.parser") >= 3)
    context.debug (dump ("CLI2::analyze defaultCommand"));
}

/*
////////////////////////////////////////////////////////////////////////////////
void CLI2::decomposeModAttributes ()
{
  bool changes = false;
  for (auto& a : _args)
  {
    if (a.hasTag ("TERMINATOR"))
      break;

    if (a.hasTag ("MODIFICATION"))
    {
      // Look for a valid attribute name.
      Nibbler n (a.attribute ("raw"));
      std::string name;
      if (n.getName (name) &&
          name.length ())
      {
        if (n.skip (':'))
        {
          std::string value;
          if (n.getQuoted   ('"', value)  ||
              n.getQuoted   ('\'', value) ||
              n.getUntilEOS (value)       ||
              n.depleted ())
          {
            if (value == "")
              value = "''";

            std::string canonical;
            if (canonicalize (canonical, "uda", name))
            {
              a.attribute ("name", canonical);
              a.attribute ("value", value);
              a.tag ("UDA");
              a.tag ("MODIFIABLE");
              changes = true;
            }

            else if (canonicalize (canonical, "attribute", name))
            {
              a.attribute ("name", canonical);
              a.attribute ("value", value);
              a.tag ("ATTRIBUTE");

              auto col = context.columns.find (canonical);
              if (col != context.columns.end () &&
                  col->second->modifiable ())
              {
                a.tag ("MODIFIABLE");
              }

              changes = true;
            }
          }
        }
      }
    }
  }

  if (changes &&
      context.config.getInteger ("debug.parser") >= 3)
    context.debug (dump ("CLI2::analyze decomposeModAttributes"));
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::decomposeModAttributeModifiers ()
{
  bool changes = false;
  for (auto& a : _args)
  {
    if (a.hasTag ("TERMINATOR"))
      break;

    if (a.hasTag ("MODIFICATION"))
    {
      // Look for a valid attribute name.
      Nibbler n (a.attribute ("raw"));
      std::string name;
      if (n.getUntil (".", name) &&
          name.length ())
      {
        std::string canonical;
        if (canonicalize (canonical, "attribute", name) ||
            canonicalize (canonical, "uda",       name))
        {
          if (n.skip ('.'))
          {
            std::string sense = "+";
            if (n.skip ('~'))
              sense = "-";

            std::string modifier;
            n.getUntilOneOf (":=", modifier);

            if (n.skip (':') ||
                n.skip ('='))
            {
              std::string value;
              if (n.getQuoted   ('"', value)  ||
                  n.getQuoted   ('\'', value) ||
                  n.getUntilEOS (value)       ||
                  n.depleted ())
              {
                if (value == "")
                  value = "''";

                std::string canonical;
                if (canonicalize (canonical, "uda", name))
                {
                  a.attribute ("name", canonical);
                  a.attribute ("modifier", modifier);
                  a.attribute ("sense", sense);
                  a.attribute ("value", value);
                  a.tag ("UDA");
                  a.tag ("MODIFIABLE");
                  changes = true;
                }

                else if (canonicalize (canonical, "attribute", name))
                {
                  a.attribute ("name", canonical);
                  a.attribute ("modifier", modifier);
                  a.attribute ("sense", sense);
                  a.attribute ("value", value);
                  a.tag ("ATTMOD");

                  auto col = context.columns.find (canonical);
                  if (col != context.columns.end () &&
                      col->second->modifiable ())
                  {
                    a.tag ("MODIFIABLE");
                  }

                  changes = true;
                }
              }
            }
          }
        }
      }
    }
  }

  if (changes &&
      context.config.getInteger ("debug.parser") >= 3)
    context.debug (dump ("CLI2::analyze decomposeModAttributeModifiers"));
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::decomposeModTags ()
{
  bool changes = false;
  for (auto& a : _args)
  {
    if (a.hasTag ("TERMINATOR"))
      break;

    if (a.hasTag ("MODIFICATION"))
    {
      Nibbler n (a.attribute ("raw"));
      std::string tag;
      std::string sign;

      if (n.getN (1, sign)             &&
          (sign == "+" || sign == "-") &&
          n.getUntilEOS (tag)          &&
          tag.find (' ') == std::string::npos)
      {
        a.attribute ("name", tag);
        a.attribute ("sign", sign);
        a.tag ("TAG");
        changes = true;
      }
    }
  }

  if (changes &&
      context.config.getInteger ("debug.parser") >= 3)
    context.debug (dump ("CLI2::analyze decomposeModTags"));
}

////////////////////////////////////////////////////////////////////////////////
void CLI2::decomposeModSubstitutions ()
{
  bool changes = false;
  for (auto& a : _args)
  {
    if (a.hasTag ("TERMINATOR"))
      break;

    if (a.hasTag ("MODIFICATION"))
    {
      std::string raw = a.attribute ("raw");
      Nibbler n (raw);
      std::string from;
      std::string to;
      bool global = false;
      if (n.getQuoted ('/', from) &&
          n.backN ()              &&
          n.getQuoted ('/', to))
      {
        if (n.skip ('g'))
          global = true;

        if (n.depleted () &&
            ! Directory (raw).exists ())
        {
          a.tag ("SUBSTITUTION");
          a.attribute ("from", from);
          a.attribute ("to", to);
          a.attribute ("global", global ? 1 : 0);
          changes = true;
        }
      }
    }
  }

  if (changes &&
      context.config.getInteger ("debug.parser") >= 3)
    context.debug (dump ("CLI2::analyze decomposeModSubstitutions"));
}

////////////////////////////////////////////////////////////////////////////////
bool CLI2::isUUIDList (const std::string& raw) const
{
  // UUIDs have a limited character set.
  if (raw.find_first_not_of ("0123456789abcdefABCDEF-,") == std::string::npos)
  {
    Nibbler n (raw);
    std::string token;
    if (n.getUUID (token) ||
        n.getPartialUUID (token))
    {
      while (n.skip (','))
        if (! n.getUUID (token) &&
            ! n.getPartialUUID (token))
          return false;

      if (n.depleted ())
        return true;
    }
  }

  return false;
}

////////////////////////////////////////////////////////////////////////////////
bool CLI2::disqualifyInsufficientTerms (
  const std::vector <std::pair <std::string, Lexer::Type>>& lexemes) const
{
  return lexemes.size () < 3 ? true : false;
}

////////////////////////////////////////////////////////////////////////////////
bool CLI2::disqualifyNoOps (
  const std::vector <std::pair <std::string, Lexer::Type>>& lexemes) const
{
  bool foundOP = false;
  for (auto& lexeme : lexemes)
    if (lexeme.second == Lexer::Type::op)
      foundOP = true;

  return ! foundOP;
}

////////////////////////////////////////////////////////////////////////////////
bool CLI2::disqualifyOnlyParenOps (
  const std::vector <std::pair <std::string, Lexer::Type>>& lexemes) const
{
  int opCount      = 0;
  int opSugarCount = 0;
  int opParenCount = 0;

  for (auto& lexeme : lexemes)
  {
    if (lexeme.second == Lexer::Type::op)
    {
      ++opCount;

      if (lexeme.first == "(" ||
          lexeme.first == ")")
        ++opParenCount;
    }

    else if (isTag          (lexeme.first) || // obsolete
             isUUIDList     (lexeme.first) ||
             isUUID         (lexeme.first) || // obsolete
             isIDSequence   (lexeme.first) || // obsolete
             isID           (lexeme.first) || // obsolete
             isPattern      (lexeme.first) || // obsolete
             isAttribute    (lexeme.first))   // obsolete
      ++opSugarCount;
  }

  return opCount == opParenCount && ! opSugarCount;
}

////////////////////////////////////////////////////////////////////////////////
// Disqualify terms when there are are binary operators at either end, as long
// as there are no operators in between, which includes syntactic sugar that
// hides operators.
bool CLI2::disqualifyFirstLastBinary (
  const std::vector <std::pair <std::string, Lexer::Type>>& lexemes) const
{
  bool firstBinary = false;
  bool lastBinary  = false;

  std::string dummy;
  if (canonicalize (dummy, "binary_operator", lexemes[0].first))
    firstBinary = true;

  if (lexemes.size () > 1 &&
      canonicalize (dummy, "binary_operator", lexemes[lexemes.size () - 1].first))
    lastBinary = true;

  return firstBinary || lastBinary;
}
*/

////////////////////////////////////////////////////////////////////////////////
