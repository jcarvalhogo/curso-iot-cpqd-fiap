# Building documentation (Doxygen)

## Requirements
- doxygen installed on your machine
  - Ubuntu: `sudo apt-get install doxygen`
  - macOS (brew): `brew install doxygen`

## Generate
From the `lib/SecureHttp` folder:

```bash
doxygen docs/Doxyfile
```

Output will be created at:

- `docs/out/html/index.html`
