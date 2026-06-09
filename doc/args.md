# Arguments

Args are URL path captures, query-string parameters, JSON body fields, or the
plain (non-file) fields of a `multipart/form-data` body. They are validated,
type-coerced, and made available as `{args.<name>}` for interpolation.

Binary inputs — a raw body or multipart file parts — are not args; they use the
`{body}` and `{files.*}` roots instead. See [binary.md](binary.md).

```yaml
endpoints:
  /search:
    get:
      args:
        q:     {min: 1, max: 100}
        limit: {type: u32, default: 20, max: 1000}
      sql: CALL Search({args.q}, {args.limit})
      return: list
```

Args are checked in this order: path captures, query string, then the body
(JSON fields, or multipart plain fields).

## Types

| Type    | Notes                                                  |
|---------|--------------------------------------------------------|
| `string` (default) | Text. Use `min`/`max` for length.           |
| `bool`  | Accepts true/false/1/0/yes/no.                         |
| `s8` `s16` `s32` `s64` | Signed integer ranges.                  |
| `u8` `u16` `u32` `u64` | Unsigned integer ranges.                |
| `int`   | Alias for `s64`.                                       |
| `number` `float` | Floating point.                               |
| `email` | RFC-style email regex.                                 |
| `uri`   | Validated URI.                                         |
| `date`  | `YYYY-MM-DD`.                                          |
| `time`  | ISO-8601 date-time with timezone.                      |
| `enum`  | One of `enum: [a, b, c]`. Implicit when `enum` is set. |
| `dict`  | Nested object. Implicit when `dict` is set.            |

## Constraints

| Key          | Applies to             | Effect                              |
|--------------|------------------------|-------------------------------------|
| `optional`   | any                    | Allow missing value.                |
| `default`    | any                    | Default value (implies optional).   |
| `min` / `max`| numbers, strings, uri, email | Numeric range or string length. |
| `pattern`    | strings                | RE2 regex the value must match.     |
| `enum`       | any                    | Restrict to a fixed list.           |
| `allow` / `deny` | any                | Per-arg access. See [access-control.md](access-control.md). |
| `source`     | any                    | Read the value from a different key in the input. |
| `help`       | any                    | Description for the OpenAPI spec.   |

## Shared and inherited args

Define reusable arg blocks under a sub-API's `args:` key:

```yaml
args:
  user:
    name:    {min: 2, max: 100}
    passkey: {pattern: "[A-Fa-f0-9]{30,32}", optional: true}
```

Reference them by name in an endpoint:

```yaml
/user:
  get:
    args: user        # pulls in name + passkey
```

A single arg can extend another with `inherit:`:

```yaml
args:
  id: {type: u32, min: 1}

endpoints:
  /item/{item}:
    args:
      item: {inherit: id, help: Item ID}
```

`inherit` can chain across sub-APIs using `<api>.<name>` (e.g.
`global.timeseries`).

## Dict args

For JSON-body args with nested structure:

```yaml
args:
  payload:
    type: dict
    dict:
      name:  {min: 1}
      value: {type: s64}
```

Access nested fields as `{args.payload.name}`.
