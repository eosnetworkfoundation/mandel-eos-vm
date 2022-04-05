(module
  (memory 1)
  (func (export "v128.select") (param v128 v128 i32) (result v128)
    (select (local.get 0) (local.get 1) (local.get 2))
  )
)

(assert_return (invoke "v128.select" (v128.const i64x2 0x0123456789ABCDEF 0xFEDCBA9876543210) (v128.const i64x2 0x0011223344556677 0x8899AABBCCDDEEFF) (i32.const 0)) (v128.const i64x2 0x0011223344556677 0x8899AABBCCDDEEFF))
(assert_return (invoke "v128.select" (v128.const i64x2 0x0123456789ABCDEF 0xFEDCBA9876543210) (v128.const i64x2 0x0011223344556677 0x8899AABBCCDDEEFF) (i32.const 1)) (v128.const i64x2 0x0123456789ABCDEF 0xFEDCBA9876543210))
