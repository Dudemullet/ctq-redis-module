# CTQ

This module is still very much in beta, I'm trying to get all ther persistance mechanisms working with both AOF and snapshots before we can get a definitive `1.0`

## Installing

- Clone this repo
- Inside the root directory run `make`
- Load the module when running your redis-server `redis-server --loadmodule main.so`
- You're all set!

## Usage

2 main methods

- `ctq.add`
- `ctq.cancel`


### `ctq.add`

Takes in 4 parameters 

- key (used if you want to latwer cancel this message)
- value
- list name
- TTL value in seconds

#### Examples

`ctq.add myKey value list 10`

This will add `value` as a message to the `list` after 10 seconds.

### `ctq.cancel`

Takes in 1 parameters, returns `OK` if key was able to be deleted. `nil` if the key was not found.

- key

#### Examples

```
ctq.add myKey myValue list 60
ctq.cancel myKey
```

This will delete the message at `myKey` and it will no longer be added to `list`. Note, cancel needs to be run before the `60` seconds expire.