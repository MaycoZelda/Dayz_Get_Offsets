# 📚 DOCUMENTATION INDEX - DayZ_Dumper_Offsets

> **Where are the offsets being saved?**
>
> ✅ In `Offsets.h` - Inline global variables inside namespaces
>
> ✅ Mechanism: Reference pointers (`m_Reference`)
>
> ✅ Saved in: `Release()` via `UpdateReference()`
>
> ✅ Accessible from: Any file that `#include "Offsets.h"`

---

## ⚙️ Build & Validation

✅ **Build Status:** SUCCESS

```text
- No compilation errors
- No critical warnings
```

## Steps

1. **Read the proper documentation** (see guide above)
2. **Test with DayZ_x64.exe**
3. **Verify that the offsets were saved**
4. **Extend with new patterns as needed**

---

##  Quick Summary

```text
Q: Where are the offsets saved?
A: In Offsets.h, as inline global variables

Q: How are they saved?
A: Via pointers in AutoOffset::UpdateReference()

Q: When are they saved?
A: In Release() after Scan()

Q: How do I use them?
A: #include "Offsets.h" and access Offsets::Namespace::Name
```

---

## 💡 Final Tip

If you have any questions, check the following in this order:

0. **Some pointers may be outdated — use Ghidra to update them**
1. **Want examples** → `COMO_USAR_OFFSETS_SALVOS.md`
2. **Want to see the code** → `ONDE_SALVAM_OFFSETS.md` + source code

🚀
