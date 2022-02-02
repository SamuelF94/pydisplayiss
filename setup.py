from distutils.core import setup, Extension

def main():
    setup(name="pydisplay",
          version="1.0.0",
          description="Python interface for framebuffer functions",
          author="SF94",
          author_email="vacationer@beautifulplace.org",
          ext_modules=[Extension("pydisplay", ["pydisplay.c"])])

if __name__ == "__main__":
    main()
