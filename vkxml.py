# Copyright 2020 Google LLC
# SPDX-License-Identifier: MIT

import xml.etree.ElementTree as ET

class VkCVar(object):
    """Parse a C-declaration like 'int* const* const blah[4]' into

    name := 'blah'
    type_name := 'int'
    type_decor.qual = 'const'
    type_decor.dim = '4'
    type_decor.ref_quals = ['', 'const']
    """

    class Decor(object):
        def __init__(self, qual, dim, bit_size, ref_quals):
            self.qual = qual
            self.dim = dim
            self.bit_size = bit_size
            self.ref_quals = ref_quals

    def __init__(self, name, type_name, type_decor=None):
        if not type_decor:
            type_decor = self.Decor(None, None, None, [])

        self.name = name
        self.type_name = type_name
        self.type_decor = type_decor

    def to_c(self, type_only):
        c_decl = self.type_name

        quals = self.type_decor.ref_quals[:]
        quals.append(self.type_decor.qual)
        for i, qual in enumerate(quals):
            is_first = i == 0
            is_last = i == len(quals) - 1

            if qual:
                # first qualifier is prepended
                if is_first:
                    c_decl = qual + ' ' + c_decl
                else:
                    c_decl = c_decl + ' ' + qual

            if is_last:
                if not type_only:
                    c_decl = c_decl + ' ' + self.name

                if self.type_decor.dim:
                    if c_decl[-1] == '*':
                        c_decl = c_decl + ' '
                    c_decl = c_decl + '[' + self.type_decor.dim + ']'

                if self.type_decor.bit_size:
                    c_decl = c_decl + ':' + self.type_decor.bit_size
            else:
                c_decl += '*'

        return c_decl

    @classmethod
    def from_c(cls, c_decl):
        """This is very limited."""
        # extract bit size
        bit_size = None
        index = c_decl.find(':')
        if index != -1:
            bit_size = c_decl[index + 1:].strip()
            c_decl = c_decl[:index]

        # extract array size
        array_size = None
        index = c_decl.find('[')
        if index != -1:
            array_size = c_decl[index + 1:c_decl.rfind(']')].strip()
            c_decl = c_decl[:index]

        # extract name
        end = len(c_decl)
        while not c_decl[end - 1].isalnum():
            end -= 1
        index = c_decl.rfind(' ', 0, end)
        name = c_decl[index + 1:end]
        c_decl = c_decl[:index]

        # extract base type which is always before the first '*'
        quals = c_decl.split('*')
        qualified_type_name = quals[0].split()
        type_name = qualified_type_name.pop()
        quals[0] = ' '.join(qualified_type_name)

        ref_quals = [qual.strip() for qual in quals]
        qual = ref_quals.pop()
        type_decor = cls.Decor(qual, array_size, bit_size, ref_quals)

        return cls(name, type_name, type_decor)

class VkVariable(object):
    def __init__(self, name, ty, attrs={}):
        self.name = name
        self.ty = ty
        self.attrs = attrs

    def maybe_null(self):
        return self.ty.is_pointer() and \
                'optional' in self.attrs and \
                self.attrs['optional'][0] == 'true'

    def is_buffer(self):
        return self.ty.indirection_depth() == 1 and \
               not self.ty.is_array() and \
               self.ty.base.name == 'void' and  \
               'len' in self.attrs

    def is_string(self):
        return self.ty.base.name == 'char' and not self.ty.is_array()

    def is_pnext(self):
        return self.name == 'pNext'

    def to_c(self):
        return VkCVar(self.name, self.ty.base.name, self.ty.decor).to_c(False)

class VkType(object):
    INCLUDE        = 0
    DEFINE         = 1
    DEFAULT        = 2
    BASETYPE       = 3
    HANDLE         = 4
    ENUM           = 5
    BITMASK        = 6
    STRUCT         = 7
    UNION          = 8
    FUNCPOINTER    = 9
    COMMAND        = 10
    DERIVED        = 11
    CATEGORY_COUNT = 12

    def __init__(self):
        self.name = None
        self.category = None
        self.base = None

        self.platforms = []
        self.aliases = []

        self.attrs = {}

        # for BASETYPE/BITMASK
        self.typedef = None

        # for HANDLE
        self.dispatchable = None

        # for ENUM (optional)
        self.enums = None

        # for BITMASK (optional)
        self.requires = None

        # for STRUCT (optional)
        self.s_type = None
        self.p_next = []

        # for FUNCPOINTER/COMMAND
        self.ret = None

        # for STRUCT/UNION/FUNCPOINTER/COMMAND
        self.variables = []

        # for DERIVED
        self.decor = None

    def init(self, name, category):
        assert(name and self.name is None)
        assert(category < self.CATEGORY_COUNT and self.category is None)
        self.name = name
        self.category = category
        if category != self.DERIVED:
            self.base = self

    def validate(self):
        assert(self.name is not None)
        assert(self.category is not None)
        assert(self.base == self.base.base)

        if self.category != self.INCLUDE:
            assert(self.base.name.isidentifier())

        if self.category == self.DERIVED:
            assert(self.base != self)
            assert(self.decor)
        else:
            assert(self.base == self)
            assert(self.decor is None)
            if self.category == self.BASETYPE and self.typedef:
                assert(self.typedef == self.typedef.base)

        if self.s_type:
            assert(self.variables[0].name == 'sType')
            assert(self.variables[1].name == 'pNext')

    def is_handle(self):
        return self.category == self.HANDLE

    def is_array(self):
        return bool(self.decor.dim) if self.decor else False

    def array_size(self):
        return self.decor.dim if self.decor else None

    def is_pointer(self):
        return bool(self.decor.ref_quals) if self.decor else False

    def indirection_depth(self):
        return len(self.decor.ref_quals) if self.decor else 0

    def is_const_array(self):
        return self.is_array() and 'const' in self.decor.qual

    def is_const_pointer(self):
        if self.is_pointer():
            for qual in self.decor.ref_quals:
                if 'const' in qual:
                    return True
        return False

    def find_variable(self, name):
        names = name.split('->')
        var_list = []
        for name in names:
            if var_list:
                candidates = var_list[-1].ty.base.variables
            else:
                candidates = self.base.variables

            found = None
            for var in candidates:
                if var.name == name:
                    found = var
                    break
            if not found:
                return []
            var_list.append(found)

        return var_list

    def c_func_ret(self):
        return self.ret.name if self.ret else 'void'

    def c_func_params(self, separator=', '):
        c_params = [var.to_c() for var in self.variables]
        return separator.join(c_params)

    def c_func_args(self, separator=', '):
        c_args = [var.name for var in self.variables]
        return separator.join(c_args)

    @staticmethod
    def _get_inner_text(elem):
        text = []
        if elem.text:
            text.append(elem.text)
        for child in elem:
            if child.tag == 'comment':
                continue
            if child.text:
                text.append(child.text)
            if child.tail:
                text.append(child.tail)
        return ' '.join(text)

    @classmethod
    def _get_type(cls, key, type_table):
        if isinstance(key, VkCVar):
            base_name = key.type_name
            name = key.to_c(True)
        else:
            base_name = key
            name = key

        if name in type_table:
            return type_table[name]

        if base_name in type_table:
            base_ty = type_table[base_name]
        else:
            base_ty = cls()
            type_table[base_name] = base_ty

        if name == base_name:
            ty = base_ty
        else:
            ty = cls()
            ty.init(name, cls.DERIVED)
            ty.base = base_ty
            ty.decor = key.type_decor
            type_table[name] = ty

        return ty

    @classmethod
    def _parse_alias(cls, elem, type_table):
        name = elem.attrib['name']
        alias = elem.attrib['alias']

        assert(name not in type_table)
        ty = cls._get_type(alias, type_table)
        ty.aliases.append(name)
        type_table[name] = ty

    @classmethod
    def _parse_bitmask(cls, type_elem, type_table):
        assert(type_elem.find('type').text == 'VkFlags')

        requires_ty = None
        if 'requires' in type_elem.attrib:
            requires = type_elem.attrib['requires']
            requires_ty = cls._get_type(requires, type_table)
        return requires_ty

    @classmethod
    def _parse_funcpointer(cls, type_elem, type_table):
        c_decls = cls._get_inner_text(type_elem).splitlines()

        # clean up the first line to abuse VkCVar
        c_decl = c_decls.pop(0)
        assert(c_decl.startswith('typedef '))
        c_decl = c_decl[8:]
        c_decl = c_decl.replace('(VKAPI_PTR * ', '', 1)
        index = c_decl.rfind('(')
        c_decl = c_decl[:index]

        c_var = VkCVar.from_c(c_decl)
        ret_ty = cls._get_type(c_var, type_table)
        if ret_ty.name == 'void':
            ret_ty = None
        name = c_var.name

        params = []
        for c_decl in c_decls:
            c_var = VkCVar.from_c(c_decl)
            param_ty = cls._get_type(c_var, type_table)
            params.append(VkVariable(c_var.name, param_ty))

        return (name, params, ret_ty)

    @classmethod
    def _parse_variable(cls, elem, type_table):
        c_decl = cls._get_inner_text(elem)
        c_var = VkCVar.from_c(c_decl)

        # sanity check
        assert(c_var.name == elem.find('name').text)
        assert(c_var.type_name == elem.find('type').text)
        enum_elem = elem.find('enum')
        if enum_elem is not None:
            assert(c_var.type_decor.dim == enum_elem.text)

        ty = cls._get_type(c_var, type_table)

        attrs = {}
        if 'values' in elem.attrib:
            attrs['values'] = elem.attrib['values'].split(',')
        if 'len' in elem.attrib:
            lens = []
            altlens = []
            if 'altlen' in elem.attrib:
                altlens = elem.attrib['altlen'].split(',')
                if altlens[-1] == 'null-terminated':
                    altlens.pop()

                for alt in altlens:
                    begin = 0
                    while not alt[begin].isalpha():
                        begin += 1
                    end = begin + 1
                    while alt[end].isalnum():
                        end += 1
                    lens.append(alt[begin:end])
            else:
                lens = elem.attrib['len'].split(',')
                if lens[-1] == 'null-terminated':
                    lens.pop()

            if lens:
                attrs['len'] = lens
            if altlens:
                attrs['altlen'] = altlens
        if 'optional' in elem.attrib:
            attrs['optional'] = elem.attrib['optional'].split(',')

        return VkVariable(c_var.name, ty, attrs)

    @classmethod
    def _parse_struct(cls, type_elem, type_table):
        members = []
        for member_elem in type_elem.iterfind('member'):
            var = cls._parse_variable(member_elem, type_table)
            members.append(var)

        s_type = None
        if members[0].name == 'sType' and 'values' in members[0].attrs:
            s_type = members[0].attrs['values'][0]

        struct_extends = []
        if 'structextends' in type_elem.attrib:
            struct_names = type_elem.attrib['structextends'].split(',')
            struct_extends = [cls._get_type(name, type_table) for
                    name in struct_names]

        returnedonly = type_elem.attrib.get(
                'returnedonly', 'false') != 'false'

        return (members, s_type, struct_extends, returnedonly)

    @classmethod
    def parse_type(cls, type_elem, type_table):
        if 'alias' in type_elem.attrib:
            cls._parse_alias(type_elem, type_table)
            return

        category = {
            'include':     cls.INCLUDE,
            'define':      cls.DEFINE,
            None:          cls.DEFAULT,
            'basetype':    cls.BASETYPE,
            'handle':      cls.HANDLE,
            'enum':        cls.ENUM,
            'bitmask':     cls.BITMASK,
            'struct':      cls.STRUCT,
            'union':       cls.UNION,
            'funcpointer': cls.FUNCPOINTER,
        }[type_elem.attrib.get('category')]

        if 'name' in type_elem.attrib:
            name = type_elem.attrib['name']
        else:
            name = type_elem.find('name').text

        ty = cls._get_type(name, type_table)
        ty.init(name, category)

        if category == cls.DEFINE:
            ty.attrs['define'] = cls._get_inner_text(type_elem)
        elif category == cls.BASETYPE:
            typedef_elem = type_elem.find('type')
            if typedef_elem is not None:
                ty.typedef = cls._get_type(typedef_elem.text, type_table)
        elif category == cls.HANDLE:
            if type_elem.find('type').text == 'VK_DEFINE_HANDLE':
                ty.dispatchable = True
        elif category == cls.BITMASK:
            to = type_elem.find('type').text
            ty.typedef = cls._get_type(to, type_table)
            requires_ty = cls._parse_bitmask(type_elem, type_table)
            ty.requires = requires_ty
        elif category == cls.STRUCT:
            members, s_type, struct_extends, returnedonly = cls._parse_struct(
                    type_elem, type_table)
            ty.variables = members
            ty.s_type = s_type
            for struct_ty in struct_extends:
                struct_ty.p_next.append(ty)
            if returnedonly:
                ty.attrs['returnedonly'] = True
        elif category == cls.UNION:
            struct = cls._parse_struct(type_elem, type_table)
            ty.variables = struct[0]
        elif category == cls.FUNCPOINTER:
            pfn, params, ret_ty = cls._parse_funcpointer(type_elem,
                    type_table)
            assert(pfn == name)
            ty.variables = params
            ty.ret = ret_ty

    @classmethod
    def parse_command(cls, command_elem, type_table):
        if 'alias' in command_elem.attrib:
            cls._parse_alias(command_elem, type_table)
            return

        name = None
        params = []
        ret_ty = None
        for child in command_elem:
            if child.tag == 'proto':
                c_decl = cls._get_inner_text(child)
                c_var = VkCVar.from_c(c_decl)
                name = c_var.name
                ret_ty = cls._get_type(c_var, type_table)
                if ret_ty.name == 'void':
                    ret_ty = None
            elif child.tag == 'param':
                var = cls._parse_variable(child, type_table)
                params.append(var)

        assert(name == command_elem.find('proto').find('name').text)

        ty = cls._get_type(name, type_table)
        ty.init(name, cls.COMMAND)
        ty.variables = params
        ty.ret = ret_ty

class VkEnums(object):
    def __init__(self, values, bitmask):
        self.values = values
        self.bitmask = bitmask

    @classmethod
    def parse_enums(cls, enums_elem, type_table):
        name = enums_elem.attrib['name']
        bitmask = enums_elem.attrib['type'] == 'bitmask'
        values = {}
        for enum_elem in enums_elem.iterfind('enum'):
            key = enum_elem.attrib['name']
            if 'value' in enum_elem.attrib:
                val = enum_elem.attrib['value']
            elif 'bitpos' in enum_elem.attrib:
                bit = int(enum_elem.attrib['bitpos'])
                val = '0x%08x' % (1 << bit)
            elif 'extnumber' in enum_elem.attrib:
                extnumber = int(enum_elem.attrib['extnumber'])
                assert(extnumber > 0)
                offset = int(enum_elem.attrib['offset'])
                val = str(1000000000 + (extnumber - 1) * 1000 + offset)
            else:
                val = values[enum_elem.attrib['alias']]
            values[key] = val

        if name in type_table:
            ty = type_table[name]
        else:
            ty = VkType()
            ty.init(name, VkType.ENUM)
            type_table[name] = ty

        if values:
            ty.enums = cls(values, bitmask)

class VkExtension(object):
    def __init__(self, api, name, number, platform, types, commands):
        self.api = api
        self.name = name

        if number.isdigit():
            self.number = int(number)
        else:
            self.number = 0

        self.platform = platform
        self.types = types
        self.commands = commands

    def is_venus(self):
        return self.api == 'venus' and self.number == 0

    def is_core(self):
        return self.api == 'vulkan' and self.number == 0

    @classmethod
    def parse_extension(cls, elem, type_table):
        api = elem.attrib.get('api', 'vulkan')
        name = elem.attrib['name']
        number = elem.attrib['number']
        platform = elem.attrib.get('platform')

        types = []
        commands = []
        for require_elem in elem.iterfind('require'):
            for child in require_elem:
                if child.tag == 'command':
                    command_name = child.attrib['name']
                    command_ty = type_table[command_name]
                    if platform:
                        command_ty.platforms.append(platform)
                    if command_ty not in commands:
                        commands.append(command_ty)
                elif child.tag == 'type':
                    type_name = child.attrib['name']
                    type_ty = type_table[type_name]
                    if platform:
                        type_ty.platforms.append(platform)
                    if type_ty not in types:
                        types.append(type_ty)

        return cls(api, name, number, platform, types, commands)

class VkApi(object):
    def __init__(self):
        self.platform_guards = {}
        self.tags = []
        self.type_table = {}
        self.extensions = []
        self.vk_xml_version = None
        self.vn_xml_version = None

    def parse_xml(self, xml):
        tree = ET.parse(xml)
        root = tree.getroot()
        for child in root:
            if child.tag == 'platforms':
                self._parse_platforms(child)
            elif child.tag == 'tags':
                self._parse_tags(child)
            elif child.tag == 'types':
                self._parse_types(child)
            elif child.tag == 'enums':
                self._parse_enums(child)
            elif child.tag == 'commands':
                self._parse_commands(child)
            elif child.tag == 'feature':
                self._parse_feature(child)
            elif child.tag == 'extensions':
                self._parse_extensions(child)

    def validate(self):
        self.vn_xml_version = self._get_xml_version(
                self.type_table['VN_HEADER_VERSION'],
                self.type_table['VN_HEADER_VERSION_COMPLETE'])

        self.vk_xml_version = self._get_xml_version(
                self.type_table['VK_HEADER_VERSION'],
                self.type_table['VK_HEADER_VERSION_COMPLETE'])

        for ty in self.type_table.values():
            ty.validate()

        command_ids = {}
        core_id_next = 0
        ext_command_id_base = 1 * 1000 * 1000 * 1000
        venus_id_next = 2 * 1000 * 1000 * 1000
        # add extension commands first
        for ext in self.extensions:
            if ext.is_venus() or ext.is_core():
                continue
            for offset, cmd in enumerate(ext.commands):
                key = 'VN_COMMAND_TYPE_' + self.uppercase_name(cmd)
                val = ext_command_id_base + (ext.number - 1) * 1000 + offset
                if key in command_ids:
                    command_ids[key].append(val)
                else:
                    command_ids[key] = [val]

        # add venus commands
        for ext in self.extensions:
            if not ext.is_venus():
                continue

            for cmd in ext.commands:
                key = 'VN_COMMAND_TYPE_' + self.uppercase_name(cmd)
                val = venus_id_next
                if key not in command_ids:
                    command_ids[key] = [val]
                    venus_id_next += 1

        # add core commands
        for ext in self.extensions:
            if not ext.is_core():
                continue

            for cmd in ext.commands:
                key = 'VN_COMMAND_TYPE_' + self.uppercase_name(cmd)
                val = core_id_next
                if key not in command_ids:
                    command_ids[key] = [val]
                    core_id_next += 1

        c_type_enums = self.type_table['VnCommandType'].enums
        for key, val in c_type_enums.values.items():
            assert(int(val) in command_ids[key])

    def uppercase_name(self, ty):
        words = []
        begin = len('vk')
        end = begin + 1
        while end < len(ty.name):
            if ty.name[end].isupper():
                words.append(ty.name[begin:end].upper())
                begin = end
                if ty.name[begin:] in self.tags:
                    break
            end += 1
        words.append(ty.name[begin:].upper())

        return '_'.join(words)

    def _parse_platforms(self, platforms_elem):
        for plat_elem in platforms_elem.iterfind('platform'):
            self.platform_guards[plat_elem.attrib['name']] = \
                    plat_elem.attrib['protect']

    def _parse_tags(self, tags_elem):
        for tag_elem in tags_elem.iterfind('tag'):
            self.tags.append(tag_elem.attrib['name'])

    def _parse_types(self, types_elem):
        for type_elem in types_elem.iterfind('type'):
            VkType.parse_type(type_elem, self.type_table)

    def _parse_enums(self, enums_elem):
        if 'type' in enums_elem.attrib:
            VkEnums.parse_enums(enums_elem, self.type_table)

    def _parse_commands(self, commands_elem):
        for command_elem in commands_elem.iterfind('command'):
            VkType.parse_command(command_elem, self.type_table)

    def _parse_feature(self, feature_elem):
        self.extensions.append(VkExtension.parse_extension(feature_elem,
            self.type_table))

    def _parse_extensions(self, extensions_elem):
        for extension_elem in extensions_elem.iterfind('extension'):
            ext = VkExtension.parse_extension(extension_elem, self.type_table)
            self.extensions.append(ext)

    def _get_xml_version(self, ver_ty, complete_ver_ty):
        ver = ver_ty.attrs['define']
        ver = ver[(ver.rindex(' ') + 1):]
        assert(ver.isdigit())

        complete_ver = complete_ver_ty.attrs['define']
        complete_ver = complete_ver[(complete_ver.rindex('(') + 1):-1]
        complete_ver = complete_ver.replace(ver_ty.name, ver)

        return 'VK_MAKE_VERSION(%s)' % complete_ver

def test():
    C_DECLS = [
        'int a',
        'int* a',
        'const int a',
        'const int* a',
        'int* const a',
        'int a[3]',
        'int* a[3]',
        'const int a[3]',
        'const int* a[3]',
        'int* const a[3]',
    ]
    for c_decl in C_DECLS:
        c_var = VkCVar.from_c(c_decl)
        assert(c_var.to_c(False) == c_decl)

if __name__ == '__main__':
    test()
