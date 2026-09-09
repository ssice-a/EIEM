"""Neutral fixtures for the read-only metadata inspection utility, not game acceptance."""
import struct
import unittest
from types import SimpleNamespace

from inspect_il2cpp_type import inspect


def fixture(stride):
    data = bytearray(2048)
    struct.pack_into('<II', data, 0, 0xFAB11BAF, 29)
    strings = b'\0NeutralController\0Test\0Evaluate\0Mock.dll\0amount\0binding\0'
    data[264:264+len(strings)] = strings
    struct.pack_into('<II',data,24,264,len(strings))
    struct.pack_into('<II',data,160,512,stride)
    struct.pack_into('<II',data,48,768,32)
    struct.pack_into('<II',data,88,1280,12)
    struct.pack_into('<II',data,96,1408,12)
    struct.pack_into('<II',data,168,1024,40)
    fields = [0]*16
    fields[0],fields[1] = strings.index(b'NeutralController'),strings.index(b'Test')
    struct.pack_into('<16I',data,512,*fields)
    struct.pack_into('<8H',data,512+stride-24,1,0,1,0,0,0,0,0)
    struct.pack_into('<6I4H',data,768,strings.index(b'Evaluate'),0,5,0,0xffffffff,0x06000001,6,0,0xffff,1)
    struct.pack_into('<3I',data,1280,strings.index(b'amount'),0x08000001,11)
    struct.pack_into('<3I',data,1408,strings.index(b'binding'),15,0x04000001)
    struct.pack_into('<10I',data,1024,strings.index(b'Mock.dll'),0,0,1,0,0,0,0,0,0)
    raw = bytearray(1024)
    base = 0x180000000
    struct.pack_into('<QQQ',raw,32,base+128,1,base+256)
    raw[128:137] = b'Mock.dll\0'
    struct.pack_into('<Q',raw,256,base+512)
    image = SimpleNamespace(data=raw,base=base,offset=lambda rva:rva)
    return bytes(data),image


class MetadataInspectionTests(unittest.TestCase):
    def test_explicit_layouts_and_token_mapping(self):
        for stride in (88,92):
            with self.subTest(stride=stride):
                data,image=fixture(stride)
                result=inspect(data,image,32,'NeutralController','Test',stride)
                method=result[0]['methods'][0]
                self.assertEqual(result[0]['assembly'],'Mock.dll')
                self.assertEqual((method['name'],method['parameter_count'],method['rva']),('Evaluate',1,'0x200'))
                self.assertEqual(method['parameters'],[dict(name='amount',token='0x8000001',type_index=11)])
                self.assertEqual(result[0]['fields'],[dict(name='binding',type_index=15,token='0x4000001')])

    def test_field_range_and_token_checked(self):
        data,image=fixture(92)
        for offset,value in ((544,1),(1416,0x08000001)):
            changed=bytearray(data)
            struct.pack_into('<I',changed,offset,value)
            with self.subTest(offset=offset),self.assertRaises(ValueError):
                inspect(changed,image,32,'NeutralController','Test',92)

    def test_parameter_range_and_token_checked(self):
        data,image=fixture(92)
        for offset,value in ((780,1),(1284,0x04000001)):
            changed=bytearray(data)
            struct.pack_into('<I',changed,offset,value)
            with self.subTest(offset=offset),self.assertRaises(ValueError):
                inspect(changed,image,32,'NeutralController','Test',92)

    def test_unknown_version_and_wrong_stride_rejected(self):
        data,image=fixture(92)
        with self.assertRaises(ValueError):
            inspect(data,image,32,'NeutralController','Test',88)
        data=bytearray(data)
        struct.pack_into('<I',data,4,31)
        with self.assertRaises(ValueError):
            inspect(data,None,None,'NeutralController','Test',92)

    def test_wrong_module_rejected(self):
        data,image=fixture(92)
        image.data[128:137]=b'Else.dll\0'
        with self.assertRaisesRegex(ValueError,'does not match'):
            inspect(data,image,32,'NeutralController','Test',92)

    def test_method_owner_and_token_checked(self):
        data,image=fixture(92)
        for offset,value in ((772,1),(788,0x06000002),(788,0x04000001)):
            changed=bytearray(data)
            struct.pack_into('<I',changed,offset,value)
            with self.subTest(offset=offset,value=value),self.assertRaises(ValueError):
                inspect(changed,image,32,'NeutralController','Test',92)

    def test_namespace_selection(self):
        data,image=fixture(92)
        self.assertEqual(inspect(data,image,32,'NeutralController','Other',92),[])

    def test_metadata_only_does_not_invent_address(self):
        data,_=fixture(92)
        result=inspect(data,None,None,'NeutralController','Test',92)
        self.assertIsNone(result[0]['methods'][0]['rva'])


if __name__=='__main__':
    unittest.main()
