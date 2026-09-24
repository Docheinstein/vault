## vault

A minimal, command-line password manager, written in C++23.

`vault` keeps your passwords (and any other secret text, such as recovery codes,
SSH passphrases or notes) in a local directory, encrypting every entry with a key
derived from a single master password. Optionally, the vault directory can be a
git repository, so that secrets can be synchronized across machines through any
git remote over HTTPS or SSH.

### Features

- One master password protects the whole vault: no keys to generate, import or back up.
- Every secret is stored in its own encrypted file; both the **name** and the **content** are encrypted.
- Single-line secrets (passwords) and multi-line secrets (notes, recovery codes, keys).
- Case-insensitive search across names and contents, with highlighted matches.
- Short, unique identifiers to refer to secrets (like git's abbreviated hashes).
- Built-in git integration: every change is committed automatically, and `clone`, `pull`
  and `push` work out of the box with HTTPS and SSH remotes.

### Comparison with pass

`vault` is inspired by [pass](https://www.passwordstore.org/), the standard unix password
manager, but aims to be easier to set up:

|                          | `vault`                                     | `pass`                                      |
|--------------------------|---------------------------------------------|---------------------------------------------|
| Encryption key           | Derived from a master password (Argon2id)   | GPG key pair                                |
| Setup                    | `vault init`, choose a password, done       | Generate/import a GPG key, configure gpg-agent, `pass init <gpg-id>` |
| New machine              | `vault clone <url>` and type the password   | Also transfer and trust the GPG private key |
| Secret names             | Encrypted (file names are opaque HMACs)     | Visible as plain file/directory names       |
| Git synchronization      | Built-in (libgit2)                          | Built-in (git executable)                   |

The main trade-off is that `pass` relies on your GPG key (and optionally a hardware token),
while the security of `vault` relies entirely on the strength of your master password:
choose a long, unique one.

### Security

Security is the primary concern of this project; the design choices are summarized below.

- **Key derivation**: the master key is derived from the master password with
  Argon2id (`crypto_pwhash` of [libsodium](https://libsodium.org)) and a random salt.
- **Encryption**: secrets are encrypted and authenticated with XSalsa20-Poly1305
  (`crypto_secretbox` of libsodium), using a fresh random nonce every time a secret is saved.
- **Per-secret keys**: each secret is encrypted with its own subkey, derived from the master key.
- **Opaque file names**: the file name of a secret is an HMAC of its name, keyed with a
  subkey of the master key, so secret names cannot be read or guessed without the password.
- **Password verification**: the master file (`<vault>/.vault`) contains no secret at all;
  it only holds the key derivation parameters and an authenticated empty message, used to
  verify that the typed master password is correct.
- **Memory hygiene**: passwords, keys and decrypted contents live in memory allocated with
  `sodium_malloc` (guarded pages, zeroed on release), are never copied into `std::string`,
  and passwords are read from the terminal with echo disabled.

### Dependencies

#### Build time

- A C++23 compiler (recent GCC or Clang)
- CMake >= 3.24
- `make`, `autoconf`, `automake` and `libtool` (to build the bundled libsodium)
- OpenSSL development headers (e.g. `openssl-devel` on Fedora, `libssl-dev` on Debian/Ubuntu),
  only when git support is enabled (used by libgit2 for HTTPS)

The following libraries are bundled as git submodules under `third_party/` and linked statically:

- [libsodium](https://github.com/jedisct1/libsodium): cryptography
- [libgit2](https://github.com/libgit2/libgit2): git support (optional)
- [args](https://github.com/Docheinstein/args): command-line argument parsing

#### Runtime

- OpenSSL (`libssl`, `libcrypto`), for HTTPS remotes
- The `ssh` executable, for SSH remotes (libgit2 is built with `USE_SSH=exec`, so it
  delegates to your system `ssh`, together with its configuration and agent)

Without git support (`-DENABLE_GIT=OFF`) there are no runtime dependencies besides the C/C++ standard libraries.

### Build

Clone the repository together with its submodules:

```shell
git clone --recurse-submodules https://github.com/Docheinstein/vault.git
cd vault
```

Configure, build and install:

```shell
mkdir build
cd build
cmake ..
make -j
sudo make install   # installs the `vault` executable (default prefix: /usr/local)
```

#### CMake Build options

| Option       | Default | Description                                                       |
|--------------|---------|-------------------------------------------------------------------|
| `ENABLE_GIT` | `ON`    | Build with libgit2 to sync secrets with a remote git repository. |

For example, to build without git support:

```shell
cmake -DENABLE_GIT=OFF ..
```

### Usage

```
vault <command> [options]
```

| Command  | Aliases          | Description                                   |
|----------|------------------|-----------------------------------------------|
| `init`   | `i`              | Create a new vault                            |
| `add`    | `a`              | Add a new secret                              |
| `list`   | `l`, `ls`        | List the names of the secrets                 |
| `show`   | `s`              | Show one or all the secrets                   |
| `search` | `g`, `grep`      | Search the secrets by name and content        |
| `edit`   | `e`              | Change the content of a secret                |
| `remove` | `r`, `rm`        | Delete a secret                               |
| `clone`  | `c`              | Clone a vault from a git remote *(git only)*  |
| `pull`   | `p`              | Pull changes from the git remote *(git only)* |
| `push`   | `u`              | Push changes to the git remote *(git only)*   |

Running `vault` without arguments prints the list of available commands.

Every command accepts `-p, --vault-path <path>` to operate on a vault other than the
default one, `~/.vault`. This allows to keep multiple independent vaults
(e.g. personal and work).

Every command that reads secrets asks for the master password first.

#### Identifiers and names

Each secret is identified by a short hexadecimal **ID**, the shortest prefix of its file
name that is unique within the vault (e.g. `3fa9`). IDs are shown by `list`, `show` and
`search` and are required by `show <id>`, `edit` and `remove`. An ID stays the same as long
as the name of the secret does not change, though it can become longer when new secrets are added.

Names are free-form, but the convention `<site>/<account>` (e.g. `github.com/alice`) is
recommended: the two components are highlighted with different colors.

#### `init`

```
vault init [-p <vault-path>]
```

Creates the vault directory (if needed) and the master file, asking for the master password.
If a vault already exists at that path, asks for confirmation before overwriting its
master file. **Overwriting it with a different password makes the existing secrets unreadable.**

With git support, it also offers to initialize a git repository in the vault directory and
to configure a remote (`origin`) by asking for its URL; leave the URL empty to skip the remote.
The master file is then committed.

#### `add`

```
vault add [-m] [-p <vault-path>]
```

Asks for the name of the secret and its content.

- By default, the content is a single line (e.g. a password), typed hidden and asked twice for confirmation.
- `-m, --multiline`: the content is read, visible, until `Ctrl+D`; useful for notes or recovery codes.

Fails if a secret with the same name already exists (use `edit` instead).

#### `list`

```
vault list [-p <vault-path>]
```

Prints the ID and name of every secret, sorted by name, without showing their contents.

```
$ vault ls
Vault password:
3fa9 github.com/alice
b21c gitlab.com/alice
e07d mail.example.com/alice
```

#### `show`

```
vault show [<id>] [-w] [-p <vault-path>]
```

Prints the name and content of the secret with the given ID or, if no ID is given, of all the secrets.

- `-w, --when`: also print the last modification time of each secret.

```
$ vault show 3fa9 -w
Vault password:
3fa9 github.com/alice
correct-horse-battery-staple
Modified at: 2026-09-24 10:32:11
```

#### `search`

```
vault search [<pattern>] [-p <vault-path>]
```

Prints the secrets whose name **or** content contains the given pattern (case-insensitive),
highlighting the matches. If the pattern is not given, it is asked interactively
(which also keeps it out of your shell history).

```
$ vault grep alice
```

#### `edit`

```
vault edit <id> [-m] [-p <vault-path>]
```

Shows the current content of the secret, then asks for the new content.

- By default, the new content is a single hidden line, asked twice for confirmation.
- `-m, --multiline`: the new content is read until `Ctrl+D`.

The name of the secret cannot be changed: to rename it, add a new secret and remove the old one.

#### `remove`

```
vault remove <id> [-p <vault-path>]
```

Deletes the secret with the given ID.

### Git synchronization

When `vault` is built with git support and the vault directory is a git repository,
`add`, `edit`, `remove` and `init` automatically stage and commit the changed files
(e.g. `Add secret 3fa9c1`). Commit messages only contain the opaque file name, never the
secret name. Commits use your git identity (`user.name` and `user.email` from the git
configuration), which must be set.

Nothing is sent to the remote until you run `vault push`.

#### `clone`

```
vault clone [<url>] [-p <vault-path>]
```

Clones an existing vault from a remote git repository into the vault path. If the URL is
not given, it is asked interactively. This is how a vault is set up on a new machine:

```shell
vault clone git@github.com:alice/my-vault.git
vault ls
```

#### `pull`

```
vault pull [-p <vault-path>]
```

Fetches from `origin` and fast-forwards the current branch to its upstream.
Only fast-forward updates are supported: if the local and remote histories have diverged,
the pull fails and must be resolved manually with `git` inside the vault directory.

#### `push`

```
vault push [-p <vault-path>]
```

Pushes the current branch to `origin`. On the first push, the upstream branch is configured
automatically (like `git push --set-upstream`).

#### HTTPS remotes

For `https://` URLs, `vault` asks for the username (unless it is part of the URL, e.g.
`https://alice@github.com/alice/my-vault.git`) and the password. With most hosting
services (GitHub, GitLab, ...) the password is a personal access token with read/write
access to the repository.

```shell
vault init   # answer "y" and use https://github.com/alice/my-vault.git as URL
vault push
```

#### SSH remotes

For SSH URLs (`git@host:user/repo.git` or `ssh://...`), `vault` runs your system `ssh`
executable, so authentication works exactly as with plain `git`: your keys in `~/.ssh`,
`ssh-agent` and `~/.ssh/config` are all honored.

```shell
vault init   # answer "y" and use git@github.com:alice/my-vault.git as URL
vault push
```

> [!TIP]
> Even though every secret is encrypted, prefer a **private** repository: it keeps the
> number of secrets and their modification history away from prying eyes.

### License

`vault` is released under the [MIT License](LICENSE).
