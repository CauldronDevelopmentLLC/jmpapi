# Binary Data

JmpAPI can receive binary request bodies (raw or multipart), store and read
binary BLOBs in the database, and return raw binary responses.

Binary values do not live in the JSON `args` namespace — JSON has no byte type.
They are exposed through two interpolation roots, `{body}` and `{files.*}`,
which may be used only where binary is meaningful: bound into a SQL query, or
written back as the response. Their byte content is never interpolated into a
string.

## Receiving a raw body

A non-multipart request body is available as `{body}`:

| Ref           | Value                                |
|---------------|--------------------------------------|
| `{body}`      | the raw request body (bytes)         |
| `{body.size}` | length in bytes                      |
| `{body.type}` | the request `Content-Type`           |

```yaml
/avatar:
  put:
    body: {max-size: 1MB, type: image/*}
    sql: CALL SetAvatar({session.user}, {body}, {body.type})
```

The optional `body:` block validates the body before the handler runs (see
[Validation](#validation)). Without it, `{body}` is still available whenever a
request carries a body.

## Receiving multipart uploads

A `multipart/form-data` request is split by each part's form-field `name`:

  - **File parts** (those with a `filename`) are exposed under `{files.<name>}`.
  - **Plain fields** (no filename) fold into the normal `{args.*}` namespace and
    validate like any other arg (see [args.md](args.md)).

| Ref                       | Value                        |
|---------------------------|------------------------------|
| `{files.<name>}`          | the part's bytes             |
| `{files.<name>.filename}` | client-supplied filename     |
| `{files.<name>.type}`     | the part's `Content-Type`    |
| `{files.<name>.size}`     | length in bytes              |

```yaml
/photos:
  post:
    args:
      caption: {max: 200}             # a plain text field of the form
    files:
      photo: {required: true, max-size: 5MB, type: [image/png, image/jpeg]}
    sql: >
      CALL AddPhoto({session.user}, {args.caption}, {files.photo},
        {files.photo.filename}, {files.photo.type})
```

## Storing blobs

A binary reference used in a query — `{body}` or `{files.<name>}` — is **bound
as a real statement parameter**, not interpolated into the SQL text. Binding is
binary-safe (NUL bytes and arbitrary content are preserved) at any size.
Ordinary text and number refs interpolate as before (see
[sql.md](sql.md#interpolation)); the two mix freely in one statement.

```yaml
sql: CALL StoreImage({args.id}, {files.image})
```

A binary ref is only valid standing alone as a whole value. Embedding it in a
larger string (`'x-{body}'`) is an error — there is nothing sensible to
interpolate bytes into.

## Returning a blob

`return: binary` writes a single value from the query result as the raw
response body: the first column of the first row.

```yaml
/image/{id}:
  get:
    args: {id: {type: u64}}
    sql:  CALL GetImage({args.id})
    return: binary
    content-type: image/png
```

The response `Content-Type` is taken from, in order:

  1. the `content-type` key — a literal or an interpolated `{ref}`; else
  2. a second selected column, if the query returns one; else
  3. `application/octet-stream`.

If the query returns no row, the response is `404`.

Blobs are not emitted by the JSON return types (`dict`, `list`, …) — a BLOB
column there would be corrupted by JSON string escaping. Select a blob only
with `return: binary`.

Static files and compiled-in assets are served by the `file` and `resource`
handlers instead — see [handlers.md](handlers.md).

## Validation

`body:` and each entry under `files:` accept:

| Key        | Effect                                                       |
|------------|--------------------------------------------------------------|
| `required` | Reject with `400` if the body/part is absent.                |
| `max-size` | Maximum byte length (e.g. `5MB`); larger → `413`.            |
| `type`     | Allowed `Content-Type`: a string, a list, or a `*` glob (e.g. `image/*`); mismatch → `415`. |
| `help`     | Description for the OpenAPI spec.                            |

Declared `body:`/`files:` also appear in the generated OpenAPI spec as the
request body (`application/octet-stream` or `multipart/form-data`).
