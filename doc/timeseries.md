# Timeseries

A timeseries runs a SQL query on a fixed period and stores each
result in a local LevelDB. Clients can fetch history or subscribe to
new values via websocket.

Requires the `timeseries-db` option (path to a LevelDB directory).

## Definition

Declared at the top level of a sub-API under `timeseries:`. Each entry
is a query plus a `period` and an optional `key`.

```yaml
timeseries:
  score:
    args: [team-id, global.timeseries]
    period: 5m
    key: team
    sql: CALL TeamGetScores()
```

| Key      | Purpose                                                    |
|----------|------------------------------------------------------------|
| `period` | How often to run the query. Human duration: `30s`, `5m`, `1h`. |
| `sql` / `query` | The query to run. See [sql.md](sql.md).             |
| `key`    | Column name (or list of names) that identifies each series. |
| `return` | `list` (default), `dict`, `one`, etc. `hlist` is not allowed. |
| `args`   | Args accepted on query/subscribe (often inherits common timeseries args). |

When `return: list`, each row is treated as one keyed datapoint and
the `key` column(s) are stripped from the stored value (so the entry
is just the value, with the time implicit).

When the new value equals the last stored value, nothing is written —
gaps mean "unchanged".

## Querying history

Once a sub-API has a `timeseries:` block, history is available via the
timeseries handler. Common args:

  - `since` (time) — only return points newer than this.
  - `max_count` (u32) — cap on points returned.

Define a shared arg block once:

```yaml
args:
  timeseries:
    since:     {type: time, optional: true}
    max_count: {type: u32, default: 100}
```

## Subscribing via websocket

Inside a `websocket` handler's `on-message`, route to a timeseries:

```yaml
/websocket:
  get:
    handler: websocket
    on-message:
      - map: msg.type
        args:
          timeseries: {enum: [team.score, user.score]}
          action:     {source: type}
          since:      {type: time, optional: true}
          max_count:  {type: u32, default: 100, max: 10000}
        to:
          subscribe:   {timeseries: '{args.timeseries}'}
          unsubscribe: {timeseries: '{args.timeseries}'}
```

The client sends e.g.
`{"type":"subscribe","timeseries":"team.score","team":42}`. JmpAPI
replies with the recent history, then pushes each new datapoint as it
is produced. `{"type":"unsubscribe", ...}` stops the stream.

Timeseries names are namespaced by sub-API: a `score` timeseries in the
`team` API is addressed as `team.score`.
