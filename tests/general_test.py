import unittest
import os
import sys

SDK_PATH = os.path.join(os.getcwd(), "install", "bin")
sys.path.append(SDK_PATH)

import pyprt

class MainTest(unittest.TestCase):
    def test_print(self): # the test functions name has to start with "test_"
        self.assertEqual(pyprt.print_val(47), 47)

    def test_PRTinitialization(self):
        pyprt.initialize_prt(SDK_PATH)
        self.assertTrue(pyprt.is_prt_initialized())

    def test_PRTshutdown(self):
        pyprt.shutdown_prt()
        self.assertFalse(pyprt.is_prt_initialized())

if __name__ == '__main__':
    unittest.main()